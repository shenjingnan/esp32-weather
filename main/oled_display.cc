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

    // ---- Step 2: 创建 LCD Panel IO 设备 ----
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

    // ---- Step 3: 安装 SSD1306 面板驱动 ----
    ESP_LOGI(TAG, "Installing SSD1306 panel driver (%dx%d)", kHorRes, kVerRes);
    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.bits_per_pixel = 1;
    panel_config.reset_gpio_num = GPIO_NUM_NC;

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {};
    ssd1306_config.height = kVerRes;
    panel_config.vendor_config = &ssd1306_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &s_panel_handle));

    // ---- Step 4: 复位、初始化、开启显示 ----
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel_handle, true));

    // ---- Step 5: 翻转显示（修正屏幕倒置问题）----
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(s_panel_handle, true, true));

    // ---- Step 6: 颜色反转 ----
    // false: 暗底亮字（bit=1 发光）
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel_handle, false));

    ESP_LOGI(TAG, "OLED init done");
}

extern "C" void oled_show_text(const char *text)
{
    if (s_panel_handle == nullptr) {
        ESP_LOGE(TAG, "OLED not initialized, call oled_init() first");
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
