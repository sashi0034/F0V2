#pragma once
#include "Array.h"
#include "Integer2D.h"

namespace TY
{
    struct GlyphBitmap
    {
        int width;
        int height;
        int left; // 描画位置調整用 (bitmap_left)
        int top; // 描画位置調整用 (bitmap_top)
        int advance; // 次の文字までのオフセット
        Array<uint8_t> buffer; // ビットマップ

        Size size() const
        {
            return Size{width, height};
        }
    };

    GlyphBitmap FreetTypeTest();
}
