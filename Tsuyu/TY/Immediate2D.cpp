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

    void Immediate2D::Text::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }
}
