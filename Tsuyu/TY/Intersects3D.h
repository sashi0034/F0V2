#pragma once
#include "PrimitiveTypes3D.h"

namespace TY
{
    // Priority:
    // - Float3
    // - Line3D
    // - Triangle3D
    // - Capsule

    // -----------------------------------------------
    // Float3

    Float3 ClosestPoint(const Float3& p, const LineSegment3D& segment);

    float DistanceSq(const Float3& p, const LineSegment3D& segment);

    float DistanceSq(const Float3& p, const Triangle3D& tri);

    // -----------------------------------------------
    // Line3D

    bool Intersects(const LineSegment3D& segment, const Triangle3D& tri);

    std::optional<Float3> IntersectsAt(const LineSegment3D& segment, const Triangle3D& tri);

    float DistanceSq(const LineSegment3D& lhs, const LineSegment3D& rhs);

    float DistanceSq(const LineSegment3D& segment, const Triangle3D& tri);

    // -----------------------------------------------
    // Triangle3D

    bool Intersects(const Triangle3D& tri, const LineSegment3D& segment);

    bool Intersects(const Triangle3D& tri, const Capsule& capsule);

    // -----------------------------------------------
    // Capsule

    bool Intersects(const Capsule& capsule, const Triangle3D& tri);
}
