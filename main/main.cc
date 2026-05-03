/* ESP32 Weather Station — WiFi 强制门户配网

   ESP32-S3 开启 WiFi APSTA 模式，首次通过 Captive Portal 配置 WiFi 凭据，
   设备连接路由器后通过 mDNS 广播域名 esp32-weather.local。
   同一局域网内的设备可通过 http://esp32-weather.local/ 访问天气信息。
*/

#include <cstdio>
#include <cstring>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mdns.h"
#include "nvs_flash.h"
#include "lwip/inet.h"

#include "dns_server.h"
#include "web_server.h"
#include "oled_display.h"

static const char *TAG = "weather_main";

// mDNS 状态
static bool s_mdns_initialized = false;

// ==================== 工具函数 ====================

static const char *get_disconnect_reason(uint8_t reason)
{
    switch (reason) {
    case 2:  return "认证无效";
    case 15: return "密码错误或认证失败";
    case 201: return "找不到指定的 WiFi";
    case 202: return "连接失败，认证不匹配";
    case 204: return "握手超时";
    default: return "连接失败";
    }
}

// ==================== mDNS 初始化 ====================

static void init_mdns(void)
{
    ESP_ERROR_CHECK(mdns_init());

    ESP_ERROR_CHECK(mdns_hostname_set(CONFIG_MDNS_HOST_NAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 Weather Station"));
    ESP_ERROR_CHECK(mdns_service_add(nullptr, "_http", "_tcp", CONFIG_WEB_SERVER_PORT, nullptr, 0));
    ESP_LOGI(TAG, "mDNS initialized: http://%s.local/", CONFIG_MDNS_HOST_NAME);
}

// 预留: 当 STA 获得 IP 后可调用此函数触发 mDNS announce
/*
static void mdns_announce_sta(void *arg)
{
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        ESP_ERROR_CHECK(mdns_netif_action(sta_netif, MDNS_EVENT_ANNOUNCE_IP4));
        ESP_LOGI(TAG, "mDNS announced on STA interface");
    }
}
*/

// ==================== 事件处理 ====================

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        auto *event = static_cast<wifi_event_ap_staconnected_t *>(event_data);
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        auto *event = static_cast<wifi_event_ap_stadisconnected_t *>(event_data);
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    } else if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "STA started");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *event = static_cast<wifi_event_sta_disconnected_t *>(event_data);
        ESP_LOGI(TAG, "STA disconnected, reason=%d", event->reason);
        s_wifi_connect_state = 3;  // failed
        snprintf(s_wifi_fail_reason, sizeof(s_wifi_fail_reason),
                 "%s (reason=%d)", get_disconnect_reason(event->reason), event->reason);

        // 更新 OLED
        char msg[64];
        snprintf(msg, sizeof(msg), "WiFi Failed:%d", event->reason);
        oled_show_text(msg);
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        auto *event = static_cast<ip_event_got_ip_t *>(event_data);
        inet_ntoa_r(event->ip_info.ip.addr, s_wifi_status_ip, sizeof(s_wifi_status_ip));
        ESP_LOGI(TAG, "STA got ip: %s", s_wifi_status_ip);
        s_wifi_connect_state = 2;  // connected

        // 更新 OLED
        char msg[64];
        snprintf(msg, sizeof(msg), "WiFi OK\n%s", s_wifi_status_ip);
        oled_show_text(msg);

        // 在 STA 获得 IP 后再初始化 mDNS
        if (!s_mdns_initialized) {
            s_mdns_initialized = true;
            init_mdns();
        }
    }
}

// ==================== WiFi 初始化 ====================

static void wifi_init_apsta(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);

    char ssid[32];
    snprintf(ssid, sizeof(ssid), "ESP32_Weather_%02X%02X%02X",
             mac[3], mac[4], mac[5]);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &ip_event_handler, nullptr));

    wifi_config_t ap_config = {};
    strcpy(reinterpret_cast<char *>(ap_config.ap.ssid), ssid);
    ap_config.ap.ssid_len = strlen(ssid);
    ap_config.ap.max_connection = 4;
    ap_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "AP started: SSID='%s' IP=%s", ssid, ip_addr);

    // OLED 显示 AP 信息
    char msg[64];
    snprintf(msg, sizeof(msg), "AP:%s", ssid);
    oled_show_text(msg);

    // 自动重连：检查 NVS 中是否有已保存的 STA 配置
    wifi_config_t existing_config = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &existing_config) == ESP_OK) {
        if (strlen(reinterpret_cast<char *>(existing_config.sta.ssid)) > 0) {
            s_wifi_connect_state = 1;  // connecting
            ESP_LOGI(TAG, "Found saved WiFi: '%s', auto-connecting...",
                     reinterpret_cast<char *>(existing_config.sta.ssid));
            oled_show_text("Connecting...");
            esp_wifi_connect();
        }
    }
}

// ==================== 主函数 ====================

extern "C" void app_main(void)
{
    // 降低 HTTP 服务器日志级别
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

    // 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化 OLED 显示
    oled_init();

    // 初始化网络栈
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 创建默认 AP 和 STA 网络接口
    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    // 初始化 WiFi APSTA 模式
    wifi_init_apsta();

    // 启动 Web 服务器（提供前端页面和 REST API）
    ESP_ERROR_CHECK(init_web_fs());
    ESP_ERROR_CHECK(start_web_server());

    // 启动 DNS 服务器（Captive Portal 核心：劫持 DNS 请求到 AP IP）
    dns_server_config_t dns_config = DNS_SERVER_CONFIG_SINGLE("*", "WIFI_AP_DEF");
    start_dns_server(&dns_config);
    ESP_LOGI(TAG, "DNS server started, captive portal ready");

    ESP_LOGI(TAG, "System ready. Connect to AP and open captive portal to configure WiFi.");
}
