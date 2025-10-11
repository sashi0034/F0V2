#pragma once
#include "CourseConstants.h"
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Race
{
    enum class CourseSegmentStyle : uint8_t
    {
        Road,
        Pipe,
        Max // end marker
    };

    struct CourseStrip
    {
        Float3 center{};
        Float3 leftmost{};
        Float3 rightmost{};

        Float3 toNext{}; // 次点へのベクトル
        Float3 normal{};

        CourseSegmentStyle style{};

        struct
        {
            std::array<Float3, PipeSubdivision> ringVectors{}; // リング上の頂点方向へのベクトル
        } pipe;
    };

    struct CourseSegment
    {
        Float3 side_p0{};
        Float3 p1{};
        Float3 p2{};
        Float3 side_p3{};

        float side_p0_roll; // radians
        float p1_roll; // radians
        float p2_roll; // radians
        float side_p3_roll; // radians

        CourseSegmentStyle style{};

        Array<CourseStrip> midwayStrips{};
    };

    struct CourseNode
    {
        Float3 pos{};
        int roll{}; // degrees
        CourseSegmentStyle style{};

        float rollRadians() const;
    };

    struct CourseData
    {
        Array<CourseNode> nodes{};
    };

    CourseData LoadCourseData(const std::string& filepath);

    void SaveCourseData(const CourseData& course, const std::string& filepath);
}
