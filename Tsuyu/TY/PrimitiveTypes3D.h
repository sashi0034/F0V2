#pragma once
#include "Vector3D.h"

namespace TY
{
    struct Plane3D
    {
        Float3 normal;
        float d;

        float signedDistanceFrom(const Float3& p) const;

        float distanceFrom(const Float3& p) const;
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

    struct LineSegment3D
    {
        Float3 p0;
        Float3 p1;
    };

    struct Capsule
    {
        Float3 p0;
        Float3 p1;
        float radius;

        [[nodiscard]]
        static Capsule AlongY(const Float3& center, float height, float radius);
    };
}
