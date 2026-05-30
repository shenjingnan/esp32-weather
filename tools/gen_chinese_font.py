#!/usr/bin/env python3
"""生成16x16中文字体点阵数据 C 头文件。

从系统字体渲染指定汉字为 16x16 单色位图，
输出可直接嵌入固件的 C 头文件。

用法: python3 gen_chinese_font.py > ../main/chinese_font.h
"""

from PIL import Image, ImageDraw, ImageFont
import sys

# 需要的汉字
CHARS = "请连接完成配网"

# 字体大小和输出格式
FONT_SIZE = 16
IMAGE_SIZE = 16


def get_font():
    """尝试加载系统中可用的中文字体"""
    candidates = [
        "/System/Library/Fonts/PingFang.ttc",
        "/Users/nemo/Library/Fonts/NotoSansHans-Medium.otf",
        "/Users/nemo/Library/Fonts/SourceHanSerifSC-Medium.otf",
    ]
    for path in candidates:
        try:
            return ImageFont.truetype(path, FONT_SIZE)
        except (IOError, OSError):
            continue
    raise RuntimeError("No Chinese font found!")


def render_char(font, char):
    """将单个汉字渲染为 16x16 单色位图，返回 32 字节数组。

    格式(与 ASCII 8x16 字体一致，MSB 为最左像素):
      每个 row (0-15) 占 2 字节: [left_byte, right_byte]
      left_byte 对应列 0-7, right_byte 对应列 8-15
    """
    img = Image.new("1", (IMAGE_SIZE, IMAGE_SIZE), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), char, font=font, fill=1)

    pixels = list(img.getdata())
    # pixels 是行主序的一维数组，值 0=黑(不显示), 1=白(显示)
    data = []
    for row in range(16):
        left_byte = 0
        right_byte = 0
        for col in range(16):
            bit = pixels[row * 16 + col]
            if bit:
                if col < 8:
                    left_byte |= 1 << (7 - col)
                else:
                    right_byte |= 1 << (7 - (col - 8))
        data.append(left_byte)
        data.append(right_byte)
    return bytes(data)


def main():
    font = get_font()

    # 收集所有字符的字体数据
    entries = []
    for ch in CHARS:
        data = render_char(font, ch)
        utf8_bytes = ch.encode("utf-8")
        entries.append((ch, utf8_bytes, data))

    # 输出 C 头文件
    out = sys.stdout

    out.write("""#pragma once

#include <cstdint>
#include <cstring>

// ====================================================
// 16x16 中文字体点阵数据
// 生成自: tools/gen_chinese_font.py
// 格式: 每个字符 32 字节, 16 行 x 2 字节/行 [left, right]
//       MSB = 最左像素 (与 ASCII 8x16 字体一致)
// ====================================================

struct ChineseFontEntry {
    const uint8_t utf8[4];   // UTF-8 编码 (最多 3 字节 + null)
    uint8_t data[32];        // 16x16 点阵数据
};

""")

    out.write("static const ChineseFontEntry s_chinese_fonts[] = {\n")
    for ch, utf8_bytes, data in entries:
        hex_str = " ".join(f"0x{b:02X}," for b in data)
        out.write(f'    {{ {{ {", ".join(f"0x{b:02X}" for b in utf8_bytes)}, 0x00 }}, {{ {hex_str} }}, }},  // {ch}\n')
    out.write("};\n\n")

    out.write(f"static constexpr int kChineseFontCount = {len(entries)};\n\n")

    out.write("""/**
 * 查找中文字体的点阵数据。
 *
 * @param utf8  指向 UTF-8 编码的中文字符 (3 字节)
 * @return      32 字节点阵指针，未找到返回 nullptr
 */
static inline const uint8_t* find_chinese_font(const char* utf8) {
    for (int i = 0; i < kChineseFontCount; i++) {
        if (reinterpret_cast<const uint8_t*>(utf8)[0] == s_chinese_fonts[i].utf8[0] &&
            reinterpret_cast<const uint8_t*>(utf8)[1] == s_chinese_fonts[i].utf8[1] &&
            reinterpret_cast<const uint8_t*>(utf8)[2] == s_chinese_fonts[i].utf8[2]) {
            return s_chinese_fonts[i].data;
        }
    }
    return nullptr;
}
""")

    print(f"// Generated {len(entries)} Chinese characters: {CHARS}", file=sys.stderr)


if __name__ == "__main__":
    main()
