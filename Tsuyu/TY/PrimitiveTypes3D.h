#pragma once
#include "Vector3D.h"

namespace TY
{
    struct Triangle3D
    {
        Float3 p0;
        Float3 p1;
        Float3 p2;

        [[nodiscard]]
        Float3 getNormal() const;

        [[nodiscard]]
        Float3 centroid() const;
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
