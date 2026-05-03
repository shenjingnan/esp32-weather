/*
 * Web Server - HTTP server startup, shutdown, and filesystem management
 */
#include "web_server.h"
#include <esp_http_server.h>
#include <esp_littlefs.h>
#include <esp_log.h>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

// HTTP 404 — 尝试从 LittleFS 服务文件，不存在则重定向到 /（Captive Portal）
static esp_err_t not_found_handler(httpd_req_t *req, httpd_err_code_t err)
{
    const char *mount = CONFIG_WEB_MOUNT_POINT;
    const char *uri = req->uri;

    // 跳过 API 路径（它们有精确匹配的 handler，不会到这里）
    if (strncmp(uri, "/api/", 5) == 0) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    // 尝试从 LittleFS 读取文件
    char filepath[512];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(filepath, sizeof(filepath), "%s%s", mount, uri);
#pragma GCC diagnostic pop

    int fd = open(filepath, O_RDONLY);
    if (fd >= 0) {
        ESP_LOGI(TAG, "404→file: %s", filepath);

        const char *type = "application/octet-stream";
        if (strstr(uri, ".html")) type = "text/html";
        if (strstr(uri, ".css"))  type = "text/css";
        if (strstr(uri, ".js"))   type = "application/javascript";
        if (strstr(uri, ".json")) type = "application/json";
        if (strstr(uri, ".png"))  type = "image/png";
        if (strstr(uri, ".svg"))  type = "image/svg+xml";
        httpd_resp_set_type(req, type);

        char buf[1024];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            httpd_resp_send_chunk(req, buf, n);
        }
        close(fd);
        httpd_resp_send_chunk(req, nullptr, 0);
        return ESP_OK;
    }

    // 文件不存在: SPA 回退或 Captive Portal 重定向
    ESP_LOGI(TAG, "404→redirect: %s", uri);
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

    // GET / — 从 LittleFS 读取 index.html 并返回
    static auto root_handler = [](httpd_req_t *req) -> esp_err_t {
        const char *mount = CONFIG_WEB_MOUNT_POINT;
        char filepath[128];
        snprintf(filepath, sizeof(filepath), "%s/index.html", mount);

        ESP_LOGI(TAG, "Serving: %s", filepath);

        int fd = open(filepath, O_RDONLY);
        if (fd < 0) {
            ESP_LOGE(TAG, "Failed to open: %s", filepath);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        httpd_resp_set_type(req, "text/html");
        char buf[512];
        ssize_t n;
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            httpd_resp_send_chunk(req, buf, n);
        }
        close(fd);
        httpd_resp_send_chunk(req, nullptr, 0);
        return ESP_OK;
    };

    httpd_uri_t root_uri = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = root_handler,
        .user_ctx  = nullptr,
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
    httpd_register_uri_handler(s_server, &root_uri);
    httpd_register_uri_handler(s_server, &api_weather_uri);
    httpd_register_uri_handler(s_server, &api_system_info_uri);
    httpd_register_uri_handler(s_server, &api_wifi_connect_uri);
    httpd_register_uri_handler(s_server, &api_system_config_uri);
    httpd_register_uri_handler(s_server, &api_wifi_scan_uri);
    httpd_register_uri_handler(s_server, &api_wifi_status_uri);

    // 注册 404 错误处理器: 优先从 LittleFS 服务文件, 否则 Captive Portal 重定向
    httpd_register_err_handler(s_server, HTTPD_404_NOT_FOUND, not_found_handler);

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
