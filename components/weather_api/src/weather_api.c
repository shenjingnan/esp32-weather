/*
 * Weather API — 彩云天气 v2.6 接口封装
 *
 * 调用 https://api.caiyunapp.com/v2.6/{apiKey}/{lng},{lat}/weather
 * 综合接口一次性获取 realtime + daily 数据。
 */
#include "weather_api.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <esp_log.h>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <cJSON.h>

static const char *TAG = "weather_api";

/* NVS 配置 */
#define NVS_NAMESPACE   "weather"
#define NVS_KEY_APIKEY  "api_key"
#define NVS_KEY_LNG     "lng"
#define NVS_KEY_LAT     "lat"

/* 彩云 API 基础 URL */
#define CAIYUN_API_HOST  "api.caiyunapp.com"
#define CAIYUN_API_PATH  "/v2.6/%s/%s,%s/weather?alert=true&dailysteps=1"

/* 默认坐标 */
#define DEFAULT_LNG  "116.3176"
#define DEFAULT_LAT  "39.9760"

/* 响应 buffer 最大大小 */
#define MAX_HTTP_RESPONSE  16384

/* ---- NVS 读写 ---- */

static esp_err_t nvs_get_str_safe(const char *key, char *out, size_t out_len, const char *default_val)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) return ret;

    size_t len = out_len;
    ret = nvs_get_str(handle, key, out, &len);
    if (ret != ESP_OK && default_val) {
        strncpy(out, default_val, out_len);
        ret = ESP_OK;
    }
    nvs_close(handle);
    return ret;
}

esp_err_t weather_save_config(const char *api_key)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_str(handle, NVS_KEY_APIKEY, api_key);
    if (ret == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return ret;
}

/* ---- HTTP 响应接收 ---- */

typedef struct {
    char *buf;
    size_t size;
    size_t capacity;
} http_response_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *resp = (http_response_t *)evt->user_data;

    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (resp->size + evt->data_len < resp->capacity) {
            memcpy(resp->buf + resp->size, evt->data, evt->data_len);
            resp->size += evt->data_len;
            resp->buf[resp->size] = '\0';
        }
        break;
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP error");
        break;
    default:
        break;
    }
    return ESP_OK;
}

/* ---- skycon 映射 ---- */

typedef struct {
    const char *skycon;
    const char *icon;
} skycon_map_t;

static const skycon_map_t skycon_table[] = {
    {"CLEAR_DAY",           "sunny"},
    {"CLEAR_NIGHT",         "sunny"},
    {"PARTLY_CLOUDY_DAY",   "cloudy"},
    {"PARTLY_CLOUDY_NIGHT", "cloudy"},
    {"CLOUDY",              "cloudy"},
    {"RAIN",                "rainy"},
    {"LIGHT_RAIN",          "rainy"},
    {"MODERATE_RAIN",       "rainy"},
    {"HEAVY_RAIN",          "rainy"},
    {"STORM_RAIN",          "rainy"},
    {"SNOW",                "snowy"},
    {"LIGHT_SNOW",          "snowy"},
    {"MODERATE_SNOW",       "snowy"},
    {"HEAVY_SNOW",          "snowy"},
    {"STORM_SNOW",          "snowy"},
    {"WIND",                "windy"},
    {"FOG",                 "foggy"},
    {"HAZE",                "foggy"},
    {"SLEET",               "snowy"},
    {"THUNDER_SHOWER",      "rainy"},
    {"SANDSTORM",           "windy"},
    {"DUST",                "windy"},
    {NULL, NULL},
};

static const char *skycon_to_icon(const char *skycon)
{
    if (!skycon) return "sunny";
    for (const skycon_map_t *m = skycon_table; m->skycon; m++) {
        if (strcmp(skycon, m->skycon) == 0) return m->icon;
    }
    return "sunny";
}

/* ---- weather_fetch 主逻辑 ---- */

esp_err_t weather_fetch(weather_data_t *out_data)
{
    if (!out_data) return ESP_ERR_INVALID_ARG;

    memset(out_data, 0, sizeof(*out_data));
    out_data->valid = false;

    /* 1. 从 NVS 读取配置 */
    char api_key[64] = {0};
    char lng[32] = DEFAULT_LNG;
    char lat[32] = DEFAULT_LAT;

    esp_err_t ret = nvs_get_str_safe(NVS_KEY_APIKEY, api_key, sizeof(api_key), NULL);
    if (ret != ESP_OK || strlen(api_key) == 0) {
        ESP_LOGW(TAG, "API key not configured");
        return ESP_ERR_INVALID_STATE;
    }

    nvs_get_str_safe(NVS_KEY_LNG, lng, sizeof(lng), DEFAULT_LNG);
    nvs_get_str_safe(NVS_KEY_LAT, lat, sizeof(lat), DEFAULT_LAT);

    /* 2. 构造 URL */
    char url[512];
    int url_len = snprintf(url, sizeof(url), CAIYUN_API_PATH, api_key, lng, lat);
    if (url_len < 0 || url_len >= sizeof(url)) {
        ESP_LOGE(TAG, "URL too long");
        return ESP_ERR_INVALID_ARG;
    }

    /* 3. 分配响应 buffer */
    char *resp_buf = calloc(1, MAX_HTTP_RESPONSE);
    if (!resp_buf) return ESP_ERR_NO_MEM;

    http_response_t http_resp = {
        .buf = resp_buf,
        .size = 0,
        .capacity = MAX_HTTP_RESPONSE,
    };

    /* 4. 配置 HTTP 客户端 */
    esp_http_client_config_t http_config = {
        .host = CAIYUN_API_HOST,
        .path = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .user_data = &http_resp,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .disable_auto_redirect = false,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    if (!client) {
        free(resp_buf);
        ESP_LOGE(TAG, "Failed to init HTTP client");
        return ESP_FAIL;
    }

    /* 5. 发起请求 */
    ret = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
        free(resp_buf);
        return ret;
    }

    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP status %d, body: %s", status_code, resp_buf);
        free(resp_buf);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Response (%d bytes): %s", (int)http_resp.size, resp_buf);

    /* 6. 解析 JSON */
    cJSON *root = cJSON_Parse(resp_buf);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse failed");
        free(resp_buf);
        return ESP_FAIL;
    }

    cJSON *status = cJSON_GetObjectItemCaseSensitive(root, "status");
    if (!cJSON_IsString(status) || strcmp(status->valuestring, "ok") != 0) {
        cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
        ESP_LOGE(TAG, "API returned error: %s",
                 cJSON_IsString(error) ? error->valuestring : "unknown");
        cJSON_Delete(root);
        free(resp_buf);
        return ESP_FAIL;
    }

    cJSON *result = cJSON_GetObjectItemCaseSensitive(root, "result");
    if (!result) {
        ESP_LOGE(TAG, "No 'result' in response");
        cJSON_Delete(root);
        free(resp_buf);
        return ESP_FAIL;
    }

    /* realtime */
    cJSON *realtime = cJSON_GetObjectItemCaseSensitive(result, "realtime");
    if (realtime) {
        cJSON *temp = cJSON_GetObjectItemCaseSensitive(realtime, "temperature");
        if (cJSON_IsNumber(temp)) out_data->temperature = (float)temp->valuedouble;

        cJSON *hum = cJSON_GetObjectItemCaseSensitive(realtime, "humidity");
        if (cJSON_IsNumber(hum)) out_data->humidity = (float)hum->valuedouble * 100.0f;

        cJSON *skycon = cJSON_GetObjectItemCaseSensitive(realtime, "skycon");
        if (cJSON_IsString(skycon)) {
            strncpy(out_data->icon, skycon_to_icon(skycon->valuestring),
                    sizeof(out_data->icon) - 1);
        }
    }

    /* forecast_keypoint 作为描述 */
    cJSON *forecast = cJSON_GetObjectItemCaseSensitive(result, "forecast_keypoint");
    if (cJSON_IsString(forecast)) {
        strncpy(out_data->description, forecast->valuestring,
                sizeof(out_data->description) - 1);
    }

    /* 位置（使用坐标） */
    snprintf(out_data->location, sizeof(out_data->location), "%s, %s", lat, lng);

    out_data->valid = true;

    cJSON_Delete(root);
    free(resp_buf);

    ESP_LOGI(TAG, "Weather: temp=%.1f, hum=%.0f%%, icon=%s, desc=%s",
             out_data->temperature, out_data->humidity,
             out_data->icon, out_data->description);

    return ESP_OK;
}
