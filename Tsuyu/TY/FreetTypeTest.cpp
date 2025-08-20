#include "pch.h"
#include "FreetTypeTest.h"

#include <freetype/freetype.h>

namespace TY
{
    GlyphBitmap FreetTypeTest()
    {
        FT_Library ft;
        if (FT_Init_FreeType(&ft))
        {
            std::cerr << "Could not init FreeType\n";
            return {};
        }

        FT_Face face;
        if (FT_New_Face(ft, "asset/font/RocknRoll/RocknRollOne-Regular.ttf", 0, &face))
        {
            std::cerr << "Failed to load font\n";
            return {};
        }

        FT_Set_Pixel_Sizes(face, 0, 48);

        // 'A' をロード
        if (FT_Load_Char(face, 'A', FT_LOAD_RENDER))
        {
            std::cerr << "Failed to load Glyph\n";
            return {};
        }

        FT_GlyphSlot g = face->glyph;
        FT_Bitmap& bmp = g->bitmap;

        GlyphBitmap glyph{};
        glyph.width = bmp.width;
        glyph.height = bmp.rows;
        glyph.left = g->bitmap_left;
        glyph.top = g->bitmap_top;
        glyph.advance = g->advance.x >> 6; // 1/64ピクセル単位を整数に

        // FreeType の bitmap は行ごとにメモリが連続しているのでコピーでOK
        glyph.buffer.resize(bmp.width * bmp.rows);
        for (int y = 0; y < bmp.rows; ++y)
        {
            std::memcpy(
                glyph.buffer.data() + y * bmp.width,
                bmp.buffer + y * bmp.pitch,
                bmp.width
            );
        }

        std::cout << "Glyph 'A': "
            << glyph.width << "x" << glyph.height
            << " left=" << glyph.left
            << " top=" << glyph.top
            << " advance=" << glyph.advance << "\n";

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        return glyph;
    }
}
