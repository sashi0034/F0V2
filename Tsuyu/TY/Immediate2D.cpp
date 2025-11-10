#include "pch.h"
#include "Immediate2D.h"

#include "ImmediateDrawer.h"

using namespace TY;

namespace
{
    ImmediateDrawer& activeImmediateDrawer()
    {
        return ImmediateDrawer::Global();
    }
}

namespace TY
{
    Immediate2D::Outline::Outline(float thickness, ColorF32 color)
        : thickness(thickness), innerColor(color), outerColor(color)
    {
    }

    Immediate2D::Outline::Outline(float thickness, ColorF32 innerColor, ColorF32 outerColor)
        : thickness(thickness), innerColor(innerColor), outerColor(outerColor)
    {
    }

    Immediate2D::Rect::Rect(const RectF& rect_)
    {
        rect = rect_;
    }

    Immediate2D::Rect& Immediate2D::Rect::setColor(const ColorF32& color_)
    {
        for (auto& c : colors)
        {
            c = color_;
        }

        return *this;
    }

    Immediate2D::Rect& Immediate2D::Rect::setOutline(const Outline& outline_)
    {
        outline = outline_;
        return *this;
    }

    void Immediate2D::Rect::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::RoundRect::RoundRect(const RectF& rect)
        : rect(rect)
    {
    }

    Immediate2D::RoundRect& Immediate2D::RoundRect::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    Immediate2D::RoundRect& Immediate2D::RoundRect::setRoundness(float roundness_, int segments_)
    {
        roundness = roundness_;
        segments = segments_;
        return *this;
    }

    Immediate2D::RoundRect& Immediate2D::RoundRect::setOutline(const Outline& outline_)
    {
        outline = outline_;
        return *this;
    }

    void Immediate2D::RoundRect::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::Line::Line(const Float2& start_, const Float2& end_)
        : start(start_), end(end_)
    {
    }

    Immediate2D::Line::Line(float x1, float y1, float x2, float y2)
        : start{x1, y1}, end{x2, y2}
    {
    }

    Immediate2D::Line& Immediate2D::Line::setThickness(float thickness_)
    {
        thickness = thickness_;
        return *this;
    }

    Immediate2D::Line& Immediate2D::Line::setColor(const ColorF32& color_)
    {
        for (auto& c : colors)
        {
            c = color_;
        }

        return *this;
    }

    Immediate2D::SquareDotLine Immediate2D::Line::asDotLine(float dotOffset) const
    {
        Immediate2D::SquareDotLine dotLine;
        dotLine.line = *this;
        dotLine.dotOffset = dotOffset;
        return dotLine;
    }

    void Immediate2D::Line::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::SquareDotLine& Immediate2D::SquareDotLine::setDotOffset(float offset_)
    {
        dotOffset = offset_;
        return *this;
    }

    void Immediate2D::SquareDotLine::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::Path::Path(const Array<Float2>& points_)
        : points(points_)
    {
    }

    Immediate2D::Path& Immediate2D::Path::append(const Float2& p)
    {
        points.push_back(p);
        return *this;
    }

    Immediate2D::Path& Immediate2D::Path::setThickness(float thickness_)
    {
        thickness = thickness_;
        return *this;
    }

    Immediate2D::Path& Immediate2D::Path::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    Immediate2D::CyclePath Immediate2D::Path::asCycle()
    {
        return CyclePath(std::move(*this));
    }

    void Immediate2D::Path::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::CyclePath::CyclePath(Path path_)
        : path(std::move(path_))
    {
    }

    void Immediate2D::CyclePath::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::Texture::Texture(const TextureHandle& handle_)
        : texture(handle_)
    {
    }

    Immediate2D::Texture& Immediate2D::Texture::setPosition(const Float2& position_, Alignment9 alignment)
    {
        position = position_;
        pivot = AlignmentToPivot(alignment);
        return *this;
    }

    Immediate2D::Texture& Immediate2D::Texture::setScale(const Float2& scale_)
    {
        scale = scale_;
        return *this;
    }

    Immediate2D::Texture& Immediate2D::Texture::resized(const Float2& size)
    {
        scale = size / texture.size().cast<Float2>();
        return *this;
    }

    void Immediate2D::Texture::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate2D::Text::Text(const FontObject& font_, const std::u32string& text_)
    {
        font = font_;
        text = text_;
    }

    Immediate2D::Text& Immediate2D::Text::setSize(float size_)
    {
        size = size_;
        return *this;
    }

    Immediate2D::Text& Immediate2D::Text::setPosition(const Float2& position_, Alignment9 alignment)
    {
        position = position_;
        pivot = AlignmentToPivot(alignment);
        return *this;
    }

    Immediate2D::Text& Immediate2D::Text::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    RectF Immediate2D::Text::build(
        const std::function<void(build_intermediate)>& callback, Float2& offsetToApply) const
    {
        const float textScaling = size.has_value() ? *size / font.fontSize() : 1.0f;

        const Size atlasSize = font.atlasTexture().size();

        Float2 penPos{position};
        Float2 regionTL{};
        Float2 regionBR{};
        const int characterCount = text.size();
        for (int i = 0; i < characterCount; ++i)
        {
            const auto& c = text[i];

            const auto& glyph = font.fetchByCodePoint(c);

            const Float2 posTL = penPos + (glyph.baselineOffset() + Point{0, font.fontSize()}) * textScaling;
            const Float2 posBR = posTL + glyph.size() * textScaling;

            const Float2 uvTL = glyph.topLeftInAtlas.cast<Float2>() / atlasSize;
            const Float2 uvBR = uvTL + glyph.size().cast<Float2>() / atlasSize;

            penPos.x += glyph.xAdvance * textScaling;

            if (i == 0)
            {
                regionTL = posTL;
            }
            else
            {
                regionTL.y = Min(regionTL.y, posTL.y);
            }

            regionBR = MaxVector2(regionBR, posBR);

            callback({posTL, posBR, uvTL, uvBR});
        }

        const SizeF regionSize = regionBR - regionTL;
        offsetToApply = -(regionTL - position) - regionSize * pivot;

        return RectF{regionTL + offsetToApply, regionSize};
    }

    Immediate2D::CachedText Immediate2D::Text::cache() const
    {
        CachedText cachedText{};

        cachedText.font = font;
        cachedText.color = color;

        Float2 offsetToApply{};
        cachedText.region =
            build([&cachedText](const Text::build_intermediate& intermediate)
                  {
                      cachedText.characters.push_back({
                          intermediate.posTL, intermediate.posBR, intermediate.uvTL, intermediate.uvBR
                      });
                  },
                  offsetToApply);

        for (auto& c : cachedText.characters)
        {
            c.posBR += offsetToApply;
            c.posTL += offsetToApply;
        }

        return cachedText;
    }

    void Immediate2D::Text::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    RectF Immediate2D::CachedText::character_type::rect() const
    {
        return RectF{posTL, posBR - posTL};
    }

    void Immediate2D::CachedText::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }
}
