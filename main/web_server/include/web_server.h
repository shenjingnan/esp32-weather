/*
 * Embedded Web Server for ESP32 Weather
 * Provides HTTP server with static file serving and REST API
 */
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_err.h>

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

/**
 * @brief GET /api/wifi/scan handler - Scan nearby WiFi APs
 */
esp_err_t rest_api_get_wifi_scan(httpd_req_t *req);

/**
 * @brief GET /api/wifi/status handler - Query WiFi connection status
 */
esp_err_t rest_api_get_wifi_status(httpd_req_t *req);

#endif /* WEB_SERVER_H */
