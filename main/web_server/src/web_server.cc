/*
 * Web Server - HTTP server startup, shutdown, and filesystem management
 */
#include "web_server.h"
#include <esp_http_server.h>
#include <esp_littlefs.h>
#include <esp_log.h>
#include <cstring>

static const char *TAG = "web_server";

static httpd_handle_t s_server = nullptr;

#ifdef CONFIG_ENABLE_WEB_SERVER

esp_err_t init_web_fs(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = CONFIG_WEB_MOUNT_POINT,
        .partition_label = "www",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS partition (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    esp_littlefs_info(conf.partition_label, &total, &used);
    ESP_LOGI(TAG, "LittleFS mounted: %d/%d bytes used (%.0f%%)",
             (int)used, (int)total, (float)used / total * 100);

    return ESP_OK;
}

// HTTP 404 — 重定向到根页面（Captive Portal 支持）
static esp_err_t captive_portal_redirect_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_set_status(req, "302 Temporary Redirect");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t start_web_server(void)
{
    if (s_server != nullptr) {
        ESP_LOGW(TAG, "Server already running");
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = CONFIG_WEB_SERVER_PORT;
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    /* Register URI handlers */
    httpd_uri_t static_file_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = fs_static_get_handler,
        .user_ctx  = const_cast<char *>(CONFIG_WEB_MOUNT_POINT),
    };

    httpd_uri_t api_weather_uri = {
        .uri       = "/api/weather",
        .method    = HTTP_GET,
        .handler   = rest_api_get_weather,
        .user_ctx  = nullptr,
    };

    httpd_uri_t api_system_info_uri = {
        .uri       = "/api/system/info",
        .method    = HTTP_GET,
        .handler   = rest_api_get_system_info,
        .user_ctx  = nullptr,
    };

    httpd_uri_t api_wifi_connect_uri = {
        .uri       = "/api/wifi/connect",
        .method    = HTTP_POST,
        .handler   = rest_api_post_wifi_connect,
        .user_ctx  = nullptr,
    };

    httpd_uri_t api_system_config_uri = {
        .uri       = "/api/system/config",
        .method    = HTTP_POST,
        .handler   = rest_api_post_system_config,
        .user_ctx  = nullptr,
    };

    httpd_uri_t api_wifi_scan_uri = {
        .uri       = "/api/wifi/scan",
        .method    = HTTP_GET,
        .handler   = rest_api_get_wifi_scan,
        .user_ctx  = nullptr,
    };

    httpd_uri_t api_wifi_status_uri = {
        .uri       = "/api/wifi/status",
        .method    = HTTP_GET,
        .handler   = rest_api_get_wifi_status,
        .user_ctx  = nullptr,
    };

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register all handlers */
    httpd_register_uri_handler(s_server, &static_file_uri);
    httpd_register_uri_handler(s_server, &api_weather_uri);
    httpd_register_uri_handler(s_server, &api_system_info_uri);
    httpd_register_uri_handler(s_server, &api_wifi_connect_uri);
    httpd_register_uri_handler(s_server, &api_system_config_uri);
    httpd_register_uri_handler(s_server, &api_wifi_scan_uri);
    httpd_register_uri_handler(s_server, &api_wifi_status_uri);

    // 注册 404 错误处理器，将未匹配请求重定向到根页面（Captive Portal）
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, captive_portal_redirect_handler);

    ESP_LOGI(TAG, "Web server started on port %d, captive portal enabled", config.server_port);
    return ESP_OK;
}

esp_err_t stop_web_server(void)
{
    if (s_server == nullptr) {
        return ESP_OK;
    }

    esp_err_t ret = httpd_stop(s_server);
    if (ret == ESP_OK) {
        s_server = nullptr;
        ESP_LOGI(TAG, "Web server stopped");
    }
    return ret;
}

#else /* !CONFIG_ENABLE_WEB_SERVER */

esp_err_t init_web_fs(void) { return ESP_OK; }
esp_err_t start_web_server(void) { return ESP_OK; }
esp_err_t stop_web_server(void) { return ESP_OK; }

#endif /* CONFIG_ENABLE_WEB_SERVER */
