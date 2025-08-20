#include "pch.h"
#include "BitmapFont.h"

#include <freetype/freetype.h>

#include "GlyphInfo.h"
#include "Grid.h"
#include "Logger.h"
#include "detail/FreeTypeContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr int padding_1 = 1;

    Size getBaseSize(int32_t fontSize)
    {
        int32_t baseWidth;
        if (fontSize <= 16) baseWidth = 512;
        else if (fontSize <= 32) baseWidth = 768;
        else if (fontSize <= 48) baseWidth = 1024;
        else if (fontSize <= 64) baseWidth = 1536;
        else if (fontSize <= 256) baseWidth = 2048;
        else baseWidth = 4096;

        const int32_t baseHeight = (fontSize <= 256 ? 256 : 512);

        return Size{baseWidth, baseHeight};
    }

    GlyphInfo stubGlyph{};
}

struct BitmapFont::Impl
{
    int m_fontSize{};

    FT_Face m_face{};

    Grid<uint8_t> m_atlasImage{};

    std::unordered_map<char32_t, GlyphInfo> m_glyphTable{};

    struct
    {
        Point pos{padding_1, padding_1};
        int maxHeightInCurrentLine{};
    } m_cursor{};

    bool m_valid{};

    Impl(const std::string& filepath, int fontSize)
        : m_fontSize(fontSize)
    {
        if (FT_New_Face(GetFreeType(), filepath.c_str(), 0, &m_face))
        {
            LogError("BitmapFont: Failed to load font from file: " + filepath);
            return;
        }

        FT_Set_Pixel_Sizes(m_face, 0, fontSize);

        m_valid = true;
    }

    const GlyphInfo& FetchGlyph(char32_t codePoint)
    {
        if (const auto it = m_glyphTable.find(codePoint); it != m_glyphTable.end())
        {
            return it->second;
        }

        // -----------------------------------------------

        if (m_atlasImage.isEmpty())
        {
            m_atlasImage = Grid<uint8_t>(getBaseSize(m_fontSize));
        }

        // -----------------------------------------------

        // TODO: FT_Load_Glyph?
        if (FT_Load_Char(m_face, codePoint, FT_LOAD_RENDER)) // TODO: FT_LOAD_COLOR?
        {
            return stubGlyph;
        }

        FT_GlyphSlot glyphSlot = m_face->glyph;
        FT_Bitmap& bitmap = glyphSlot->bitmap;

        GlyphInfo glyph{};
        glyph.glyphIndex = codePoint;
        glyph.width = bitmap.width;
        glyph.height = bitmap.rows;
        glyph.left = glyphSlot->bitmap_left;
        glyph.top = glyphSlot->bitmap_top;
        glyph.xAdvance = glyphSlot->advance.x / 64.0f;
        glyph.yAdvance = glyphSlot->advance.y / 64.0f;

        // -----------------------------------------------

        if (m_cursor.pos.x + glyph.width + padding_1 >= m_atlasImage.width())
        {
            m_cursor.pos.x = padding_1;
            m_cursor.pos.y += m_cursor.maxHeightInCurrentLine + padding_1;
            m_cursor.maxHeightInCurrentLine = glyph.height;
        }
        else
        {
            m_cursor.maxHeightInCurrentLine = Max<int>(m_cursor.maxHeightInCurrentLine, glyph.height);
        }

        if (m_cursor.pos.y + glyph.height + padding_1 >= m_atlasImage.height())
        {
            // TODO: Handle atlas overflow
            LogError("BitmapFont: Atlas image is too small to fit the glyph.");
            return stubGlyph;
        }

        glyph.topLeftInAtlas = m_cursor.pos;

        for (int y = 0; y < glyph.height; ++y)
        {
            std::memcpy(&m_atlasImage[glyph.topLeftInAtlas.y + y][glyph.topLeftInAtlas.x],
                        bitmap.buffer + y * bitmap.pitch,
                        glyph.width);
        }

        m_cursor.pos.x += glyph.width + padding_1;

        // -----------------------------------------------

        m_glyphTable[codePoint] = glyph;
        return m_glyphTable[codePoint];
    }
};

namespace TY
{
    BitmapFont::BitmapFont(const std::string& filepath, int fontSize)
        : p_impl(std::make_shared<Impl>(filepath, fontSize))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    const GlyphInfo& BitmapFont::fetchByCodePoint(char32_t codePoint) const
    {
        return p_impl ? p_impl->FetchGlyph(codePoint) : stubGlyph;
    }

    const Grid<uint8_t>& BitmapFont::atlasImage() const
    {
        if (p_impl)
        {
            return p_impl->m_atlasImage;
        }
        else
        {
            static const Grid<uint8_t> empty{};
            return empty;
        }
    }
}
