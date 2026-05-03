/*
 * Embedded Web Server for ESP32 Weather
 * Provides HTTP server with static file serving and REST API
 */
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_err.h>
#include <esp_http_server.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LittleFS and mount www partition
 * @return ESP_OK on success
 */
esp_err_t init_web_fs(void);

/**
 * @brief Start the embedded HTTP server
 * @return ESP_OK on success
 */
esp_err_t start_web_server(void);

/**
 * @brief Stop the HTTP server and unmount filesystem
 * @return ESP_OK on success
 */
esp_err_t stop_web_server(void);

/* ---- WiFi 连接状态（由 main.cc 事件回调更新，rest_api.cc 和 web_server.cc 读取）---- */

/** WiFi 连接状态: 0=idle, 1=connecting, 2=connected, 3=failed */
extern volatile int s_wifi_connect_state;

/** STA IP 地址字符串（connected 时有效） */
extern char s_wifi_status_ip[16];

/** 连接失败原因描述 */
extern char s_wifi_fail_reason[64];

/* ---- REST API 处理函数 ---- */

/** GET /api/wifi/scan - Scan nearby WiFi APs */
esp_err_t rest_api_get_wifi_scan(httpd_req_t *req);

/** GET /api/wifi/status - Query WiFi connection status */
esp_err_t rest_api_get_wifi_status(httpd_req_t *req);

/** POST /api/wifi/connect - Save WiFi credentials and connect */
esp_err_t rest_api_post_wifi_connect(httpd_req_t *req);

/** GET /api/weather - Return weather data */
esp_err_t rest_api_get_weather(httpd_req_t *req);

/** GET /api/system/info - Return device system info */
esp_err_t rest_api_get_system_info(httpd_req_t *req);

/** POST /api/system/config - Save system configuration */
esp_err_t rest_api_post_system_config(httpd_req_t *req);

/* ---- 静态文件处理 ---- */

/** GET wildcard - Serve static files from LittleFS */
esp_err_t fs_static_get_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif

#endif /* WEB_SERVER_H */
