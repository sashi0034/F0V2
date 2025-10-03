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

        float p1_roll; // radians
        float p2_roll; // radians

        Array<Float3> midwayPositions{};
        Array<CourseStrip> midwayStrips{};
    };

    struct CourseNode
    {
        Float3 pos{};
        int roll{}; // degrees

        float rollRadians() const;
    };

    struct CourseData
    {
        Array<CourseNode> nodes{};
    };

    CourseData LoadCourseData(const std::string& filepath);

    void SaveCourseData(const CourseData& course, const std::string& filepath);
}
