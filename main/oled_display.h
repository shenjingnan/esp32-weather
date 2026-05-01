#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化 OLED 显示屏 (SSD1306, I2C, 128x64)
 *
 * 硬件连接:
 *   XIAO D4 (GPIO5) → OLED SDA
 *   XIAO D5 (GPIO6) → OLED SCL
 *   XIAO 3V3        → OLED VCC
 *   XIAO GND        → OLED GND
 *
 * 调用后屏幕被初始化并开启显示。
 */
void oled_init(void);

/**
 * 在 OLED 屏幕上居中显示文字（8x16 字体）
 */
void oled_show_text(const char *text);

/**
 * 清空 OLED 屏幕
 */
void oled_clear(void);

#ifdef __cplusplus
}
#endif
