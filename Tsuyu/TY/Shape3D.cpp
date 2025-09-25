#include "pch.h"
#include "Shape3D.h"

namespace TY
{
    Shape3D::Line::Line(const Float3& start_, const Float3& end_)
        : start(start_), end(end_)
    {
    }

    Shape3D::Line& Shape3D::Line::setColor(const ColorF32& color)
    {
        colors[0] = color;
        colors[1] = color;
        return *this;
    }

    Shape3D::Line& Shape3D::Line::setColor(const ColorF32& c0, const ColorF32& c1)
    {
        colors[0] = c0;
        colors[1] = c1;
        return *this;
    }
}
