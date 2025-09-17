#include "pch.h"
#include "Shape2D.h"

#include "ShapeDrawer.h"

using namespace TY;

namespace
{
    ShapeDrawer& activeShapeDrawer()
    {
        return ShapeDrawer::Global();
    }
}

namespace TY
{
    Shape2D::Outline::Outline(float thickness, ColorF32 color)
        : thickness(thickness), innerColor(color), outerColor(color)
    {
    }

    Shape2D::Outline::Outline(float thickness, ColorF32 innerColor, ColorF32 outerColor)
        : thickness(thickness), innerColor(innerColor), outerColor(outerColor)
    {
    }

    Shape2D::Rect::Rect(const RectF& rect_)
    {
        rect = rect_;
    }

    Shape2D::Rect& Shape2D::Rect::setColor(const ColorF32& color_)
    {
        for (auto& c : colors)
        {
            c = color_;
        }

        return *this;
    }

    Shape2D::Rect& Shape2D::Rect::setOutline(const Outline& outline_)
    {
        outline = outline_;
        return *this;
    }

    void Shape2D::Rect::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::RoundRect::RoundRect(const RectF& rect)
        : rect(rect)
    {
    }

    Shape2D::RoundRect& Shape2D::RoundRect::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    Shape2D::RoundRect& Shape2D::RoundRect::setRoundness(float roundness_, int segments_)
    {
        roundness = roundness_;
        segments = segments_;
        return *this;
    }

    Shape2D::RoundRect& Shape2D::RoundRect::setOutline(const Outline& outline_)
    {
        outline = outline_;
        return *this;
    }

    void Shape2D::RoundRect::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::Line::Line(const Float2& start_, const Float2& end_)
        : start(start_), end(end_)
    {
    }

    Shape2D::Line::Line(float x1, float y1, float x2, float y2)
        : start{x1, y1}, end{x2, y2}
    {
    }

    Shape2D::Line& Shape2D::Line::setThickness(float thickness_)
    {
        thickness = thickness_;
        return *this;
    }

    Shape2D::Line& Shape2D::Line::setColor(const ColorF32& color_)
    {
        for (auto& c : colors)
        {
            c = color_;
        }

        return *this;
    }

    Shape2D::SquareDotLine Shape2D::Line::asDotLine(float dotOffset) const
    {
        Shape2D::SquareDotLine dotLine;
        dotLine.line = *this;
        dotLine.dotOffset = dotOffset;
        return dotLine;
    }

    void Shape2D::Line::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::SquareDotLine& Shape2D::SquareDotLine::setDotOffset(float offset_)
    {
        dotOffset = offset_;
        return *this;
    }

    void Shape2D::SquareDotLine::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::Path::Path(const Array<Float2>& points_)
        : points(points_)
    {
    }

    Shape2D::Path& Shape2D::Path::append(const Float2& p)
    {
        points.push_back(p);
        return *this;
    }

    Shape2D::Path& Shape2D::Path::setThickness(float thickness_)
    {
        thickness = thickness_;
        return *this;
    }

    Shape2D::Path& Shape2D::Path::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    Shape2D::CyclePath Shape2D::Path::asCycle()
    {
        return CyclePath(std::move(*this));
    }

    void Shape2D::Path::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::CyclePath::CyclePath(Path path_)
        : path(std::move(path_))
    {
    }

    void Shape2D::CyclePath::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape2D::Text::Text(const FontObject& font_, const std::u32string& text_)
    {
        font = font_;
        text = text_;
    }

    Shape2D::Text& Shape2D::Text::setSize(float size_)
    {
        size = size_;
        return *this;
    }

    Shape2D::Text& Shape2D::Text::setPosition(const Float2& position_, Alignment9 alignment)
    {
        position = position_;
        pivot = AlignmentToPivot(alignment);
        return *this;
    }

    Shape2D::Text& Shape2D::Text::setColor(const ColorF32& color_)
    {
        color = color_;
        return *this;
    }

    void Shape2D::Text::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }
}
