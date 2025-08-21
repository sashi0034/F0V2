#pragma once
#include "Integer2D.h"

namespace TY
{
    using GlyphIndex = uint64_t;

    struct GlyphInfo
    {
        GlyphIndex glyphIndex{};

        int16_t left{};

        int16_t top{};

        int16_t width{};

        int16_t height{};

        float xAdvance{};

        float yAdvance{};

        Point topLeftInAtlas{};

        // -----------------------------------------------

        Point baselineOffset() const
        {
            return Point{left, -top};
        }

        Size size() const
        {
            return Size{width, height};
        }
    };
}
