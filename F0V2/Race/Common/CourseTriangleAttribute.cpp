#include "pch.h"

#include "CourseTriangleAttribute.h"

namespace Race
{
    // std::array<Float2, 3> TrianglePatternUtil::GetUvTable(const pattern_t& pattern)
    // {
    //     switch (pattern)
    //     {
    //     case CourseTriangleAttribute::Triangle_00_10_11:
    //         return {Float2{0, 0}, Float2{1, 0}, Float2{1, 1}};
    //     case CourseTriangleAttribute::Triangle_00_11_01:
    //         return {Float2{0, 0}, Float2{1, 1}, Float2{0, 1}};
    //     case CourseTriangleAttribute::Triangle_10_01_00:
    //         return {Float2{1, 0}, Float2{0, 1}, Float2{0, 0}};
    //     case CourseTriangleAttribute::Triangle_10_11_01:
    //         return {Float2{1, 0}, Float2{1, 1}, Float2{0, 1}};
    //     default:
    //         assert(false);
    //         return {};
    //     }
    // }

    std::array<Float3, 4> TrianglePatternUtil::ArrangePoints_00_10_01_11(
        const Float3& p0, const Float3& p1, const Float3& p2, const GroundTriangleAttribute& attr)
    {
        switch (attr.pattern)
        {
        case GroundTriangleAttribute::Triangle_00_10_11: // p3: 01
            return {
                /* 00: */ p0, /* 10: */ p1, /* 01: */ attr.p3, /* 11: */ p2
            };
        case GroundTriangleAttribute::Triangle_00_11_01: // p3: 10
            return {
                /* 00: */ p0, /* 10: */ attr.p3, /* 01: */ p2, /* 11: */ p1
            };
        case GroundTriangleAttribute::Triangle_10_01_00:
            return {
                /* 00: */ p2, /* 10: */ p0, /* 01: */ p1, /* 11: */ attr.p3
            };
        case GroundTriangleAttribute::Triangle_10_11_01:
            return {
                /* 00: */ attr.p3, /* 10: */ p0, /* 01: */ p2, /* 11: */ p1
            };
        default:
            assert(false);
            return {};
        }
    }
}
