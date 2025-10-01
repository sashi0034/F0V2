#pragma once
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct CourseStrip
    {
        Float3 center;
        Float3 leftmost;
        Float3 rightmost;

        Float3 toNext; // 次点へのベクトル
        Float3 normal;
    };

    struct CourseSegment
    {
        Float3 p1{};
        Float3 p2{};

        Array<Float3> midwayPositions{};
        Array<CourseStrip> midwayStrips{};
    };

    struct CourseData
    {
        Array<CourseSegment> segments{};
    };
}
