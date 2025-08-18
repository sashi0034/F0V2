#include "pch.h"
#include "Shape2D.h"

namespace TY
{
    Shape2D::Rectangle::Rectangle(const RectF& rect_)
    {
        rect = rect_;
    }

    Shape2D::Rectangle& Shape2D::Rectangle::setColor(const ColorF32& color_)
    {
        for (auto& c : colors)
        {
            c = color_;
        }

        return *this;
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

    Shape2D::SquareDotLine& Shape2D::SquareDotLine::setDotOffset(float offset_)
    {
        dotOffset = offset_;
        return *this;
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
}
