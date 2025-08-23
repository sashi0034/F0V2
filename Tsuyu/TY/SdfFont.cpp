#include "pch.h"
#include "SdfFont.h"

#include <freetype/freetype.h>

#include "DynamicTexture.h"
#include "GlyphInfo.h"
#include "Grid.h"
#include "Logger.h"
#include "Rect.h"
#include "detail/FreeTypeContext.h"
#include "detail/RenderEventComponent.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr GlyphInfo stubGlyph{};

    struct DistanceFieldElement
    {
        bool dirty;

        /// @brief ピクセルが存在する領域からの距離
        float distance;

        void write(float d)
        {
            dirty = true;
            distance = d;
        }
    };

    struct QueueElement
    {
        Point nextPoint;
    };

    constexpr std::array directionOnes = {Point{1, 0}, Point{0, 1}, Point{-1, 0}, Point{0, -1}};

    struct BitmapView
    {
        uint8_t* data;
        int pitch;
        int width;
        int height;

        // TODO: padding

        bool inBounds(const Point& p) const
        {
            return 0 <= p.x && p.x < width && 0 <= p.y && p.y < height;
        }

        uint8_t operator[](const Point& p) const
        {
            return data[p.y * pitch + p.x];
        }
    };

    void exploreAndPushForTransparent(
        Grid<DistanceFieldElement>& distanceField,
        std::deque<QueueElement>& queue,
        const BitmapView& bitmap,
        const Point& p)
    {
        for (int i = 0; i < directionOnes.size(); ++i)
        {
            const auto p1 = p.movedBy(directionOnes[i]);
            const bool isTransparent = bitmap.inBounds(p1) && bitmap[p1] == 0;
            if (not isTransparent) continue;

            const auto nextDistance = distanceField[p].distance + 1;
            if (distanceField[p1].dirty == false)
            {
                distanceField[p1].write(nextDistance);
                queue.push_back(QueueElement{p1});
            }
        }
    }

    void exploreAndPushForNonTransparent(
        Grid<DistanceFieldElement>& distanceField,
        std::deque<QueueElement>& queue,
        const BitmapView& bitmap,
        const Point& p)
    {
        for (int i = 0; i < directionOnes.size(); ++i)
        {
            const auto p1 = p.movedBy(directionOnes[i]);
            const bool nonTransparent = bitmap.inBounds(p1) && bitmap[p1] != 0;
            if (not nonTransparent) continue;

            if (distanceField[p1].dirty == false)
            {
                const auto nextDistance = distanceField[p].distance - 1;
                distanceField[p1].write(nextDistance);
                queue.push_back(QueueElement{p1});
            }
        }
    }

    void makeDistanceField(Grid<uint8_t>& atlasImage, const Rect& region, const BitmapView& bitmap)
    {
        const auto fieldSize = atlasImage.size();
        auto distanceField = Grid<DistanceFieldElement>{fieldSize};

        std::deque<QueueElement> queue{};

        // 初回探索
        for (int x = 0; x < fieldSize.x; ++x)
        {
            for (int y = 0; y < fieldSize.y; ++y)
            {
                Point p{x, y};
                if (atlasImage[p] != 0)
                {
                    // 不透明ピクセル
                    exploreAndPushForTransparent(distanceField, queue, bitmap, p);
                }
                else
                {
                    // 透明ピクセル
                    distanceField[p].distance = 1;
                    exploreAndPushForNonTransparent(distanceField, queue, bitmap, p);
                }
            }
        }

        // キューがなくなるまで探索
        while (queue.size() > 0)
        {
            const auto element = queue.front();
            queue.pop_front();

            exploreAndPushForTransparent(distanceField, queue, bitmap, element.nextPoint);
            exploreAndPushForNonTransparent(distanceField, queue, bitmap, element.nextPoint);
        }

        float minDistance{-1};
        float maxDistance{1};
        for (const auto& element : distanceField)
        {
            if (element.dirty)
            {
                minDistance = Min(minDistance, element.distance);
                maxDistance = Max(maxDistance, element.distance);
            }
        }

        for (int x = region.leftX(); x < region.rightX(); ++x)
        {
            for (int y = region.topY(); y < region.bottomY(); ++y)
            {
                float d = distanceField[y - region.topY()][x - region.leftX()].distance;
                d = (d - minDistance) / (maxDistance - minDistance);
                atlasImage[Point{x, y}] = 255 - d * 255;
            }
        }
    }
}

struct SdfFont::Impl : RenderEvent::Lister
{
    int m_fontSize{};

    FT_Face m_face{};

    int m_atlasPadding{};

    Grid<uint8_t> m_atlasImage{};

    DynamicTexture m_atlasTexture{};

    std::unordered_map<char32_t, GlyphInfo> m_glyphTable{};

    struct
    {
        Point pos{};
        int maxHeightInCurrentLine{};
    } m_cursor{};

    bool m_shouldUpdateAtlas{};

    bool m_valid{};

    Impl(const std::string& filepath, int fontSize, const SdfFontOptions& options)
        : m_fontSize(fontSize), m_atlasPadding(options.atlasPadding)
    {
        m_cursor.pos = Point{m_atlasPadding, m_atlasPadding};

        if (FT_New_Face(GetFreeType(), filepath.c_str(), 0, &m_face))
        {
            LogError("SdfFont: Failed to load font from file: " + filepath);
            return;
        }

        FT_Set_Pixel_Sizes(m_face, 0, fontSize);

        m_atlasImage = Grid<uint8_t>(Size{options.atlasSize, options.atlasSize});
        m_atlasTexture = DynamicTexture(getAtlasImageView());

        m_valid = true;
    }

    const GlyphInfo& FetchGlyph(char32_t codePoint)
    {
        if (const auto it = m_glyphTable.find(codePoint); it != m_glyphTable.end())
        {
            return it->second;
        }

        // -----------------------------------------------

        if (FT_Load_Char(m_face, codePoint, FT_LOAD_RENDER))
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

        if (m_cursor.pos.x + glyph.width + m_atlasPadding >= m_atlasImage.width())
        {
            m_cursor.pos.x = m_atlasPadding;
            m_cursor.pos.y += m_cursor.maxHeightInCurrentLine + m_atlasPadding;
            m_cursor.maxHeightInCurrentLine = glyph.height;
        }
        else
        {
            m_cursor.maxHeightInCurrentLine = Max<int>(m_cursor.maxHeightInCurrentLine, glyph.height);
        }

        if (m_cursor.pos.y + glyph.height + m_atlasPadding >= m_atlasImage.height())
        {
            // TODO: Handle atlas overflow
            LogError("SdfFont: Atlas image is too small to fit the glyph.");
            return stubGlyph;
        }

        glyph.topLeftInAtlas = m_cursor.pos;

        makeDistanceField(
            m_atlasImage,
            Rect{m_cursor.pos, Size{glyph.width, glyph.height}},
            BitmapView{bitmap.buffer, bitmap.pitch, static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows)}
        );

        m_cursor.pos.x += glyph.width + m_atlasPadding;

        // -----------------------------------------------

        m_shouldUpdateAtlas = true;

        m_glyphTable[codePoint] = glyph;
        return m_glyphTable[codePoint];
    }

    Array<GlyphInfo> fetchByString(const std::u32string& str)
    {
        Array<GlyphInfo> glyphs;
        for (const char32_t codePoint : str)
        {
            glyphs.push_back(FetchGlyph(codePoint));
        }

        return glyphs;
    }

    void beforeFlush() override
    {
        if (m_shouldUpdateAtlas)
        {
            m_atlasTexture.upload(getAtlasImageView());
        }
    }

private:
    ImageView getAtlasImageView()
    {
        return ImageView(
            m_atlasImage.data(),
            m_atlasImage.size(),
            m_atlasImage.size_in_bytes(),
            DXGI_FORMAT_R8_UNORM
        );
    }
};

namespace TY
{
    SdfFont::SdfFont(const std::string& filepath, int fontSize, const SdfFontOptions& options)
        : p_impl(std::make_shared<Impl>(filepath, fontSize, options))
    {
        if (p_impl->m_valid)
        {
            RenderEvent::AddLister(p_impl);
        }
        else
        {
            p_impl.reset();
        }
    }

    const GlyphInfo& SdfFont::fetchByCodePoint(char32_t codePoint) const
    {
        return p_impl ? p_impl->FetchGlyph(codePoint) : stubGlyph;
    }

    Array<GlyphInfo> SdfFont::fetchByString(const std::u32string& str) const
    {
        return p_impl ? p_impl->fetchByString(str) : Array<GlyphInfo>{};
    }

    int SdfFont::fontSize() const
    {
        return p_impl ? p_impl->m_fontSize : 0;
    }

    // const Grid<uint8_t>& SdfFont::atlasImage() const
    // {
    //     if (p_impl)
    //     {
    //         return p_impl->m_atlasImage;
    //     }
    //     else
    //     {
    //         static const Grid<uint8_t> empty{};
    //         return empty;
    //     }
    // }

    TextureResource SdfFont::atlasTexture() const
    {
        return p_impl ? p_impl->m_atlasTexture.getResource() : TextureResource{};
    }
}
