#pragma once
#include "PrimitiveTypes3D.h"

namespace TY
{
    // Priority:
    // - Float3
    // - Aabb3D
    // - Line3D
    // - LineSegment3D
    // - Plane3Points
    // - Triangle3D
    // - Quad3D
    // - Capsule

    // -----------------------------------------------
    // Float3

    Float3 ClosestPoint(const Float3& p, const LineSegment3D& segment);

    float DistanceSq(const Float3& lhs, const Float3& rhs);

    float DistanceSq(const Float3& p, const LineSegment3D& segment);

    float DistanceSq(const Float3& p, const Plane3Points& plane);

    float DistanceSq(const Float3& p, const Triangle3D& tri);

    float DistanceSq(const Float3& p, const Quad3D& quad);

    // -----------------------------------------------

    bool Intersects(const Aabb3D& lhs, const Aabb3D& rhs);

    // -----------------------------------------------
    // Line3D

    std::optional<Float3> IntersectsAt(const Line3D& line, const Plane3D& plane, float* hitDistance = nullptr);

    // -----------------------------------------------
    // LineSegment3D

    bool Intersects(const LineSegment3D& segment, const Triangle3D& tri);

    bool Intersects(const LineSegment3D& segment, const Quad3D& quad);

    std::optional<Float3> IntersectsAt(const LineSegment3D& segment, const Triangle3D& tri);

    float DistanceSq(const LineSegment3D& lhs, const LineSegment3D& rhs);

    float DistanceSq(const LineSegment3D& segment, const Triangle3D& tri);

    float DistanceSq(const LineSegment3D& segment, const Quad3D& quad);

    // -----------------------------------------------
    // Triangle3D

    bool Intersects(const Triangle3D& tri, const LineSegment3D& segment);

    bool Intersects(const Triangle3D& tri, const Capsule& capsule);

    // -----------------------------------------------
    // Quad3D

    bool Intersects(const Quad3D& quad, const Capsule& capsule);

    // -----------------------------------------------
    // Capsule

    bool Intersects(const Capsule& capsule, const Triangle3D& tri);

    bool Intersects(const Capsule& capsule, const Quad3D& quad);
}
