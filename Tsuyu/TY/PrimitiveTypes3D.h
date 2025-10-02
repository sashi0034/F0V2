#pragma once
#include "Vector3D.h"

namespace TY
{
    struct Plane3D
    {
        Float3 normal;
        float d;

        [[nodiscard]]
        float signedDistanceFrom(const Float3& p) const;

        [[nodiscard]]
        float distanceFrom(const Float3& p) const;

        [[nodiscard]]
        Float3 projection(const Float3& p) const;
    };

    struct Aabb3D
    {
        Float3 min;
        Float3 max;

        Aabb3D stretched(float length) const;

        float volume() const;
    };

    struct Plane3Points
    {
        Float3 p0;
        Float3 p1;
        Float3 p2;

        [[nodiscard]]
        Float3 getNormal() const;
    };

    struct Triangle3D
    {
        Float3 p0;
        Float3 p1;
        Float3 p2;

        [[nodiscard]]
        Triangle3D movedBy(const Float3& v) const;

        [[nodiscard]]
        Float3 getAreaNormal() const;

        [[nodiscard]]
        Float3 getNormal() const;

        [[nodiscard]]
        Float3 centroid() const;

        [[nodiscard]]
        Plane3D asPlane() const;

        [[nodiscard]]
        Plane3Points asPlane3Points() const;
    };

    struct Quad3D
    {
        // p0 ----- p1
        // |         |
        // |         |
        // p3 ----- p2

        Float3 p0;
        Float3 p1;
        Float3 p2;
        Float3 p3;

        [[nodiscard]]
        bool isPlanar() const;

        [[nodiscard]]
        bool isValid() const;

        [[nodiscard]]
        Float3 getNormal() const;

        [[nodiscard]]
        Float3 arithmeticCenter() const;
    };

    struct Line3D
    {
        Float3 point;
        Float3 normalizedDir;

        Line3D() = default;

        Line3D(const Float3& point, const Float3& normalizedDir);

        static Line3D FromPoints(const Float3& from, const Float3& to);
    };

    struct LineSegment3D
    {
        Float3 p0;
        Float3 p1;

        Aabb3D aabb() const;
    };

    struct Capsule
    {
        Float3 p0;
        Float3 p1;
        float radius;

        Aabb3D aabb() const;

        [[nodiscard]]
        static Capsule AlongY(const Float3& center, float height, float radius);
    };
}
