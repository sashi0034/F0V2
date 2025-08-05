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
}
