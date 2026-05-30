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

/**
 * 开始水平滚动显示文字（marquee 效果）
 *
 * 创建 FreeRTOS 任务持续从右向左循环滚动文字，
 * 直到 oled_stop_scroll() 被调用。
 * 仅支持 ASCII 字符，非 ASCII 会被替换为 '?'。
 *
 * @param text  要滚动显示的文字内容
 */
void oled_start_scroll(const char *text);

/**
 * 停止滚动并清屏
 */
void oled_stop_scroll(void);

#ifdef __cplusplus
}
#endif
