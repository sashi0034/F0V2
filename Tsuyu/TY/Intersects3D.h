#pragma once
#include "PrimitiveTypes3D.h"
#include "Triangle3D.h"

namespace TY
{
    // Priority:
    // - Float3
    // - Line3D
    // - Triangle3D
    // - Capsule

    // -----------------------------------------------
    // Float3

    Float3 ClosetBetween(const Float3& p, const Line3D& line);

    // -----------------------------------------------
    // Line3D
    bool Intersects(const Line3D& line, const Triangle3D& tri);

    std::optional<Float3> IntersectsAt(const Line3D& line, const Triangle3D& tri);

    float DistanceSq(const Line3D& lhs, const Line3D& rhs);

    Float3 ClosestPoint(const Float3& p, const Line3D& line);

    // -----------------------------------------------
    // Triangle3D
    bool Intersects(const Triangle3D& tri, const Line3D& line);

    bool Intersects(const Triangle3D& tri, const Capsule& capsule);

    // -----------------------------------------------
    // Capsule
    bool Intersects(const Capsule& capsule, const Triangle3D& tri);
}
