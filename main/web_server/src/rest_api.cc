/*
 * REST API handlers - backend endpoints for Preact frontend
 */
#include <esp_http_server.h>
#include <esp_log.h>
#include <cJSON.h>
#include <cstring>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_timer.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include <esp_chip_info.h>
#endif
#include <esp_system.h>

#include "web_server.h"

static const char *TAG = "rest_api";

/* ---- WiFi 连接状态（全局变量，由 main.cc 事件回调更新）---- */
volatile int s_wifi_connect_state = 0;  /* 0=idle, 1=connecting, 2=connected, 3=failed */
char s_wifi_status_ip[16] = {0};
char s_wifi_fail_reason[64] = {0};

/* Helper: send JSON response */
static esp_err_t send_json_response(httpd_req_t *req, int code, cJSON *json)
{
    const char *response = cJSON_PrintUnformatted(json);
    httpd_resp_set_status(req, code == 200 ? "200 OK" : (code == 400 ? "400 Bad Request" : "500 Internal Server Error"));
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);
    free(const_cast<char *>(response));
    cJSON_Delete(json);
    return ESP_OK;
}

/* Helper: convert RSSI to signal bars (0-4) */
static int rssi_to_bars(int rssi)
{
    if (rssi >= -50) return 4;
    if (rssi >= -60) return 3;
    if (rssi >= -70) return 2;
    if (rssi >= -80) return 1;
    return 0;
}

/* Helper: check if WiFi requires authentication */
static bool is_encrypted(wifi_auth_mode_t auth_mode)
{
    return auth_mode != WIFI_AUTH_OPEN;
}

/* GET /api/weather - Return weather data */
esp_err_t rest_api_get_weather(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "location", "Beijing");           // TODO: from NVS config
    cJSON_AddNumberToObject(root, "temperature", 22);               // TODO: from weather_api component
    cJSON_AddNumberToObject(root, "humidity", 65);                  // TODO: from weather_api component
    cJSON_AddStringToObject(root, "description", "晴转多云");
    cJSON_AddStringToObject(root, "icon", "sunny");

    return send_json_response(req, 200, root);
}

/* GET /api/system/info - Return device system info */
esp_err_t rest_api_get_system_info(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    char model[32];
    snprintf(model, sizeof(model), "ESP32-S%d", chip_info.revision);
    cJSON_AddStringToObject(root, "chipModel", model);
#else
    cJSON_AddStringToObject(root, "chipModel", "ESP32-C6");
#endif

    cJSON_AddNumberToObject(root, "freeHeap", esp_get_free_heap_size());
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(root, "wifiRssi", -45);                // TODO: from wifi_manager
    cJSON_AddItemToObject(root, "batteryMv", cJSON_CreateNull()); // TODO: from power_management
    cJSON_AddStringToObject(root, "firmwareVersion", "0.1.0");

    return send_json_response(req, 200, root);
}

/* POST /api/wifi/connect - Save WiFi credentials and connect */
esp_err_t rest_api_post_wifi_connect(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = nullptr;
    int received = 0;

    if (total_len >= 4096) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request too large");
        return ESP_FAIL;
    }

    buf = static_cast<char *>(calloc(1, total_len + 1));
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);

    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(root, "ssid");
    cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");

    if (!cJSON_IsString(ssid) || strlen(ssid->valuestring) == 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing 'ssid' field");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "WiFi connect request: SSID='%s'", ssid->valuestring);

    // 更新连接状态
    s_wifi_connect_state = 1;  // connecting
    s_wifi_status_ip[0] = '\0';
    s_wifi_fail_reason[0] = '\0';

    // 配置 STA 并连接
    wifi_config_t sta_config = {};
    strncpy(reinterpret_cast<char *>(sta_config.sta.ssid),
            ssid->valuestring, sizeof(sta_config.sta.ssid) - 1);

    const char *pwd = cJSON_IsString(password) ? password->valuestring : "";
    if (strlen(pwd) > 0) {
        strncpy(reinterpret_cast<char *>(sta_config.sta.password),
                pwd, sizeof(sta_config.sta.password) - 1);
        sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        sta_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }

    // 保存 SSID 字符串，因为 cJSON_Delete 后会释放
    char ssid_buf[33] = {0};
    strncpy(ssid_buf, ssid->valuestring, sizeof(ssid_buf) - 1);

    cJSON_Delete(root);

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set STA config: %s", esp_err_to_name(ret));
        s_wifi_connect_state = 3;  // failed
        snprintf(s_wifi_fail_reason, sizeof(s_wifi_fail_reason), "配置失败: %s", esp_err_to_name(ret));

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "code", 500);
        cJSON_AddStringToObject(resp, "message", "Failed to set WiFi config");
        send_json_response(req, 500, resp);
        return ESP_OK;
    }

    // 确保 APSTA 模式
    esp_wifi_set_mode(WIFI_MODE_APSTA);

    // 发起连接
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect: %s", esp_err_to_name(ret));
        s_wifi_connect_state = 3;  // failed
        snprintf(s_wifi_fail_reason, sizeof(s_wifi_fail_reason), "连接发起失败: %s", esp_err_to_name(ret));

        cJSON *resp = cJSON_CreateObject();
        cJSON_AddNumberToObject(resp, "code", 500);
        cJSON_AddStringToObject(resp, "message", "Failed to initiate WiFi connection");
        send_json_response(req, 500, resp);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "WiFi connection initiated for SSID: '%s'", ssid_buf);

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "code", 200);
    cJSON_AddStringToObject(resp, "message", "WiFi connection initiated");
    send_json_response(req, 200, resp);

    return ESP_OK;
}

/* POST /api/system/config - Save system configuration (e.g., API key) */
esp_err_t rest_api_post_system_config(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = nullptr;
    int received = 0;

    if (total_len >= 2048) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request too large");
        return ESP_FAIL;
    }

    buf = static_cast<char *>(calloc(1, total_len + 1));
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive body");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    free(buf);

    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *apiKey = cJSON_GetObjectItemCaseSensitive(root, "apiKey");

    if (cJSON_IsString(apiKey)) {
        ESP_LOGI(TAG, "API Key configured (length=%d)", strlen(apiKey->valuestring));
        /* TODO: Save to NVS via nvs_config component */
    }

    cJSON *resp = cJSON_CreateObject();
    cJSON_AddNumberToObject(resp, "code", 200);
    cJSON_AddStringToObject(resp, "message", "Configuration saved");
    send_json_response(req, 200, resp);

    return ESP_OK;
}

/* GET /api/wifi/scan - Scan nearby WiFi access points */
esp_err_t rest_api_get_wifi_scan(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {
        .ssid = nullptr,
        .bssid = nullptr,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 0,
                .max = 120,
            }
        },
    };

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(ret));
        cJSON *err_resp = cJSON_CreateObject();
        cJSON_AddStringToObject(err_resp, "error", "scan_failed");
        send_json_response(req, 500, err_resp);
        return ESP_OK;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    if (ap_count == 0) {
        cJSON *empty_arr = cJSON_CreateArray();
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        return send_json_response(req, 200, empty_arr);
    }

    if (ap_count > 20) {
        ap_count = 20;
    }

    wifi_ap_record_t ap_records[20];
    uint16_t actual_count = ap_count;
    esp_wifi_scan_get_ap_records(&actual_count, ap_records);

    cJSON *arr = cJSON_CreateArray();
    for (int i = 0; i < actual_count; i++) {
        char escaped_ssid[128] = {0};
        const char *src = reinterpret_cast<const char *>(ap_records[i].ssid);
        int epos = 0;
        for (int j = 0; src[j] && epos < static_cast<int>(sizeof(escaped_ssid)) - 2; j++) {
            if (src[j] == '"' || src[j] == '\\') {
                if (epos < static_cast<int>(sizeof(escaped_ssid)) - 2) {
                    escaped_ssid[epos++] = '\\';
                }
            }
            if (static_cast<unsigned char>(src[j]) >= 0x20) {
                escaped_ssid[epos++] = src[j];
            }
        }

        cJSON *ap_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(ap_obj, "ssid", escaped_ssid);
        cJSON_AddNumberToObject(ap_obj, "rssi", ap_records[i].rssi);
        cJSON_AddNumberToObject(ap_obj, "bars", rssi_to_bars(ap_records[i].rssi));
        cJSON_AddBoolToObject(ap_obj, "encrypted", is_encrypted(ap_records[i].authmode));
        cJSON_AddNumberToObject(ap_obj, "channel", ap_records[i].primary);
        cJSON_AddItemToArray(arr, ap_obj);
    }

    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return send_json_response(req, 200, arr);
}

/* GET /api/wifi/status - Query current WiFi connection status */
esp_err_t rest_api_get_wifi_status(httpd_req_t *req)
{
    const char *status_str;
    switch (s_wifi_connect_state) {
        case 1:  status_str = "connecting"; break;
        case 2:  status_str = "connected";  break;
        case 3:  status_str = "failed";     break;
        default: status_str = "idle";       break;
    }

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "status", status_str);
    cJSON_AddStringToObject(root, "ip", s_wifi_status_ip);
    cJSON_AddStringToObject(root, "reason", s_wifi_fail_reason);

    return send_json_response(req, 200, root);
}
