/*
 * Weather API — 彩云天气 v2.6 接口封装
 */
#ifndef WEATHER_API_H
#define WEATHER_API_H

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 天气数据结构体 */
typedef struct {
    float temperature;
    float humidity;
    char description[64];
    char icon[32];
    char location[72];
    bool valid;
} weather_data_t;

/**
 * @brief 从彩云天气 API 获取天气数据
 * 从 NVS 读取 api_key 和坐标（有默认值），调用彩云综合接口，
 * 解析返回的 JSON 并填充 weather_data_t。
 *
 * @param out_data 输出天气数据，调用者分配内存
 * @return ESP_OK 表示 HTTP 请求完成（无论 API 返回 ok 还是失败），
 *         ESP_FAIL 表示网络错误等
 */
esp_err_t weather_fetch(weather_data_t *out_data);

/**
 * @brief 保存 API Key 到 NVS
 * @param api_key 彩云天气 API token
 * @return ESP_OK on success
 */
esp_err_t weather_save_config(const char *api_key);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_API_H */
