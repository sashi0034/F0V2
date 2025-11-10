#pragma once
#include "Integer2D.h"
#include "Vector2D.h"

namespace TY
{
    using GlyphIndex = uint64_t;

    struct GlyphInfo
    {
        GlyphIndex glyphIndex{};

        float left{};

        float top{};

        int16_t width{};

        int16_t height{};

        float xAdvance{};

        float yAdvance{};

        Point topLeftInAtlas{};

        // -----------------------------------------------

        Float2 baselineOffset() const
        {
            return Float2{left, -top};
        }

        Size size() const
        {
            return Size{width, height};
        }
    };
}
