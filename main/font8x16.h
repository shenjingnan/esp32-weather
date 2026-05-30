#pragma once
#include <cstdint>

/**
 * 在 1bpp 显存缓冲区中绘制字符串（8x16 字体）
 *
 * @param fb       显存指针（SSD1306 垂直页寻址格式，大小 hor_res * ver_res / 8 字节）
 * @param hor_res  水平分辨率（像素）
 * @param ver_res  垂直分辨率（像素）
 * @param str      要绘制的 ASCII 字符串
 * @param x        起始 X 坐标（像素，左上角为原点）
 * @param y        起始 Y 坐标（像素，文本基线位置）
 */
void font8x16_draw_string(uint8_t *fb, int hor_res, int ver_res,
                          const char *str, int x, int y);

/**
 * 在 1bpp 显存缓冲区中绘制混合字符串（ASCII 8x16 + 中文 16x16）
 *
 * 自动识别 UTF-8 编码:
 *   - 单字节 (< 0x80) → 8x16 ASCII 字体渲染，宽度 8px
 *   - 三字节 (0xE0-0xEF) → 查 chinese_font.h 16x16 字体渲染，宽度 16px
 *   其他情况渲染为 '?'
 *
 * @param fb       显存指针
 * @param hor_res  水平分辨率
 * @param ver_res  垂直分辨率
 * @param str      UTF-8 混合字符串
 * @param x        起始 X 坐标
 * @param y        基线 Y 坐标
 * @return         字符串总像素宽度 (用于滚动计算)
 */
int font_draw_mixed(uint8_t *fb, int hor_res, int ver_res,
                    const char *str, int x, int y);
