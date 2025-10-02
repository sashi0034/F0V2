#include "pch.h"
#include "Shape3D.h"

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

    void Shape3D::Line::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }

    Shape3D::LineSet& Shape3D::LineSet::setColor(const ColorF32& color)
    {
        colors[0] = color;
        colors[1] = color;
        return *this;
    }

    Shape3D::LineSet& Shape3D::LineSet::appendLine(const Float3& start, const Float3& end)
    {
        lines.emplace_back(start, end);
        return *this;
    }

    Shape3D::LineSet& Shape3D::LineSet::appendTriangle(const Triangle3D& tri)
    {
        lines.emplace_back(tri.p0, tri.p1);
        lines.emplace_back(tri.p1, tri.p2);
        lines.emplace_back(tri.p2, tri.p0);
        return *this;
    }

    Shape3D::LineSet& Shape3D::LineSet::appendAabb(const Aabb3D& aabb)
    {
        const Float3& min = aabb.min;
        const Float3& max = aabb.max;

        lines.emplace_back(Float3{min.x, min.y, min.z}, Float3{max.x, min.y, min.z});
        lines.emplace_back(Float3{max.x, min.y, min.z}, Float3{max.x, max.y, min.z});
        lines.emplace_back(Float3{max.x, max.y, min.z}, Float3{min.x, max.y, min.z});
        lines.emplace_back(Float3{min.x, max.y, min.z}, Float3{min.x, min.y, min.z});

        lines.emplace_back(Float3{min.x, min.y, max.z}, Float3{max.x, min.y, max.z});
        lines.emplace_back(Float3{max.x, min.y, max.z}, Float3{max.x, max.y, max.z});
        lines.emplace_back(Float3{max.x, max.y, max.z}, Float3{min.x, max.y, max.z});
        lines.emplace_back(Float3{min.x, max.y, max.z}, Float3{min.x, min.y, max.z});

        lines.emplace_back(Float3{min.x, min.y, min.z}, Float3{min.x, min.y, max.z});
        lines.emplace_back(Float3{max.x, min.y, min.z}, Float3{max.x, min.y, max.z});
        lines.emplace_back(Float3{max.x, max.y, min.z}, Float3{max.x, max.y, max.z});
        lines.emplace_back(Float3{min.x, max.y, min.z}, Float3{min.x, max.y, max.z});

        return *this;
    }

    void Shape3D::LineSet::pushAuto()
    {
        (void)activeShapeDrawer().push(*this);
    }
}
