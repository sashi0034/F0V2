#pragma once
#include "TY/Vector3D.h"

namespace Race
{
    struct CourseTriangleAttribute
    {
        enum triangle_pattern : uint8_t
        {
            Triangle_01_00_11,
            Triangle_11_00_10,
        };

        triangle_pattern pattern;

        std::array<Float3, 4> normals_00_10_01_11{}; // FIXME: 現在は全く同じ normals データが三角形二つ分存在するという冗長な実装である

        Float3 p3;
    };
}
