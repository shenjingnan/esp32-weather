/* XIAO ESP32-S3 SSD1306 OLED 显示模块
 *
 *   通过 I2C 接口驱动 0.96 寸白色 OLED 显示屏（SSD1306 芯片）
 *
 *   硬件连接（XIAO ESP32-S3）:
 *     SDA: D4 (GPIO5)
 *     SCL: D5 (GPIO6)
 *     VCC: 3V3
 *     GND: GND
 *
 *   屏幕规格：128x64 像素，I2C 地址 0x3C
 */

#include "oled_display.h"

#include <cstring>
#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "font8x16.h"

static const char *TAG = "oled";

// ========== 硬件引脚定义 ==========
static constexpr int kI2cPort          = 0;
static constexpr gpio_num_t kSdaGpio   = GPIO_NUM_5;    // XIAO D4
static constexpr gpio_num_t kSclGpio   = GPIO_NUM_6;    // XIAO D5
static constexpr uint8_t kI2cAddr      = 0x3C;           // SSD1306 I2C 地址
static constexpr int kI2cClkHz         = 400 * 1000;     // 400 kHz

// ========== 屏幕参数 ==========
static constexpr int kHorRes = 128;
static constexpr int kVerRes = 64;   // 0.96 寸为 128x64

// 显存缓冲区：1bpp，SSD1306 垂直页寻址格式，128*64/8 = 1024 字节
static uint8_t s_framebuffer[kHorRes * kVerRes / 8];

// ========== 驱动句柄 ==========
static esp_lcd_panel_handle_t s_panel_handle = nullptr;

extern "C" void oled_init(void)
{
    ESP_LOGI(TAG, "=== SSD1306 OLED Init ===");
    ESP_LOGI(TAG, "Screen: %dx%d, I2C addr: 0x%02X, SDA=GPIO%d, SCL=GPIO%d",
             kHorRes, kVerRes, kI2cAddr, kSdaGpio, kSclGpio);

    // ---- Step 1: 创建 I2C 主机总线 ----
    ESP_LOGI(TAG, "Initializing I2C bus");
    i2c_master_bus_handle_t i2c_bus = nullptr;
    i2c_master_bus_config_t bus_config = {
        .i2c_port        = static_cast<i2c_port_t>(kI2cPort),
        .sda_io_num      = kSdaGpio,
        .scl_io_num      = kSclGpio,
        .clk_source      = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags           = { .enable_internal_pullup = true },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    // ---- Step 2: 探测 OLED 设备（地址 0x3C）----
    // 没有外接 OLED 模块时优雅跳过，系统继续运行
    if (i2c_master_probe(i2c_bus, kI2cAddr, 100) != ESP_OK) {
        ESP_LOGW(TAG, "No OLED found at 0x%02X, display disabled", kI2cAddr);
        s_panel_handle = nullptr;
        ESP_LOGW(TAG, "OLED disabled, system continues without display");
        return;
    }

    // ---- Step 3: 安装 SSD1306（设备已确认存在，出错即硬件故障）----
    ESP_LOGI(TAG, "Installing panel IO (addr=0x%02X)", kI2cAddr);
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr            = kI2cAddr,
        .control_phase_bytes = 1,
        .dc_bit_offset       = 6,
        .lcd_cmd_bits        = 8,
        .lcd_param_bits      = 8,
        .scl_speed_hz        = kI2cClkHz,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Installing SSD1306 panel driver (%dx%d)", kHorRes, kVerRes);
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.bits_per_pixel = 1;
    panel_config.reset_gpio_num = GPIO_NUM_NC;

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {};
    ssd1306_config.height = kVerRes;
    panel_config.vendor_config = &ssd1306_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &s_panel_handle));

    ESP_LOGI(TAG, "Resetting and initializing panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, true, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, false));

    ESP_LOGI(TAG, "OLED init done");
}

extern "C" void oled_show_text(const char *text)
{
    if (s_panel_handle == nullptr) {
        return;
    }

    memset(s_framebuffer, 0, sizeof(s_framebuffer));

    int text_len = strlen(text);
    int x = (kHorRes - text_len * 8) / 2;   // 水平居中
    int y = (kVerRes - 16) / 2 + 16;         // 垂直居中（基线位置）

    font8x16_draw_string(s_framebuffer, kHorRes, kVerRes, text, x, y);

    ESP_LOGI(TAG, "Drawing '%s' at (%d, %d)", text, x, y);
    ESP_ERROR_CHECK(
        esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, kHorRes, kVerRes, s_framebuffer)
    );
}

extern "C" void oled_clear(void)
{
    if (s_panel_handle == nullptr) {
        return;
    }
    memset(s_framebuffer, 0, sizeof(s_framebuffer));
    ESP_ERROR_CHECK(
        esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, kHorRes, kVerRes, s_framebuffer)
    );
}

// ========== 滚动显示 ==========

static TaskHandle_t s_scroll_task = nullptr;
static volatile bool s_scroll_active = false;
static char s_scroll_text[128] = {};

static void scroll_task(void *arg)
{
    (void)arg;
    const int y = (kVerRes - 16) / 2 + 16;  // 垂直居中基线

    // 先渲染一次到临时缓冲区以计算文本总宽度
    uint8_t tmp_fb[kHorRes * kVerRes / 8] = {};
    int total_width = font_draw_mixed(tmp_fb, kHorRes, kVerRes,
                                      s_scroll_text, 0, y);
    int scroll_x = kHorRes;  // 从屏幕右端外侧开始

    while (s_scroll_active) {
        memset(s_framebuffer, 0, sizeof(s_framebuffer));

        total_width = font_draw_mixed(s_framebuffer, kHorRes, kVerRes,
                                      s_scroll_text, scroll_x, y);

        ESP_ERROR_CHECK(
            esp_lcd_panel_draw_bitmap(s_panel_handle, 0, 0, kHorRes, kVerRes, s_framebuffer)
        );

        scroll_x -= 2;                     // 每次左移 2 像素
        if (scroll_x < -total_width) {
            scroll_x = kHorRes;             // 全部滚出左端后循环
        }

        vTaskDelay(pdMS_TO_TICKS(30));     // ~33fps
    }

    s_scroll_task = nullptr;
    vTaskDelete(nullptr);
}

extern "C" void oled_start_scroll(const char *text)
{
    if (s_panel_handle == nullptr) {
        return;
    }

    // 如果有正在运行的滚动任务，先停止
    if (s_scroll_task != nullptr) {
        oled_stop_scroll();
    }

    strncpy(s_scroll_text, text, sizeof(s_scroll_text) - 1);
    s_scroll_text[sizeof(s_scroll_text) - 1] = '\0';

    s_scroll_active = true;
    BaseType_t ret = xTaskCreate(scroll_task, "oled_scroll", 3072,
                                 nullptr, 5, &s_scroll_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create scroll task");
        s_scroll_active = false;
        s_scroll_task = nullptr;
    }
}

extern "C" void oled_stop_scroll(void)
{
    if (s_scroll_task == nullptr) {
        return;
    }

    s_scroll_active = false;

    // 等待任务退出
    TickType_t timeout = xTaskGetTickCount() + pdMS_TO_TICKS(200);
    while (s_scroll_task != nullptr && xTaskGetTickCount() < timeout) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    s_scroll_task = nullptr;
    oled_clear();
}
