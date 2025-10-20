#pragma once
#include "TY/PrimitiveTypes3D.h"
#include "TY/PrimitiveTypes3D.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct CourseTriangleAttribute
    {
        enum triangle_pattern : uint8_t
        {
            // 00-11 対角線
            Triangle_00_10_11,
            Triangle_00_11_01,

            // 10-01 対角線
            Triangle_10_01_00,
            Triangle_10_11_01,
        };

        typedef uint8_t triangle_pattern_t;

        triangle_pattern pattern;

        std::array<Float3, 4> normals_00_10_01_11{}; // FIXME: 現在は全く同じ normals データが三角形二つ分存在するという冗長な実装である

        Float3 p3;
    };

    namespace TrianglePatternUtil
    {
        using pattern_t = CourseTriangleAttribute::triangle_pattern_t;

        [[nodiscard]]
        std::array<Float2, 3> GetUvTable(const pattern_t& pattern);

        [[nodiscard]]
        std::array<Float3, 4> ArrangePoints_00_10_01_11(
            const Float3& p0, const Float3& p1, const Float3& p2, const CourseTriangleAttribute& attr);
    }
}
