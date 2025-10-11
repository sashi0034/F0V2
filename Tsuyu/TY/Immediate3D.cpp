#include "pch.h"
#include "Immediate3D.h"

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
    Immediate3D::Line::Line(const Float3& start_, const Float3& end_)
        : start(start_), end(end_)
    {
    }

    Immediate3D::Line& Immediate3D::Line::setColor(const ColorF32& color)
    {
        colors[0] = color;
        colors[1] = color;
        return *this;
    }

    Immediate3D::Line& Immediate3D::Line::setColor(const ColorF32& c0, const ColorF32& c1)
    {
        colors[0] = c0;
        colors[1] = c1;
        return *this;
    }

    void Immediate3D::Line::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }

    Immediate3D::LineSet& Immediate3D::LineSet::setColor(const ColorF32& color)
    {
        colors[0] = color;
        colors[1] = color;
        return *this;
    }

    Immediate3D::LineSet& Immediate3D::LineSet::appendLine(const Float3& start, const Float3& end)
    {
        lines.emplace_back(start, end);
        return *this;
    }

    Immediate3D::LineSet& Immediate3D::LineSet::appendTriangle(const Triangle3D& tri)
    {
        lines.emplace_back(tri.p0, tri.p1);
        lines.emplace_back(tri.p1, tri.p2);
        lines.emplace_back(tri.p2, tri.p0);
        return *this;
    }

    Immediate3D::LineSet& Immediate3D::LineSet::appendAabb(const Aabb3D& aabb)
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

    void Immediate3D::LineSet::pushAuto()
    {
        (void)activeImmediateDrawer().push(*this);
    }
}
