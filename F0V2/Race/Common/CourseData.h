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
        Cylinder,
        Gap,
        Max // end marker
    };

    enum class CourseGimmickKind : uint8_t
    {
        Barrier,
        BoostPad_C,
        JumpPad_C,
        Max // end marker
    };

    struct CourseStrip
    {
        Float3 center{};
        Float3 leftmost{};
        Float3 rightmost{};
        float width{};

        Float3 toNext{}; // 次点へのベクトル
        float lengthToNext{};
        Float3 normal{};

        CourseSegmentStyle style{};

        // TODO: pipe を外す
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

        float side_leftWidth0;
        float leftWidth1;
        float leftWidth2;
        float side_leftWidth3;

        float side_rightWidth0;
        float rightWidth1;
        float rightWidth2;
        float side_rightWidth3;

        CourseSegmentStyle style{};

        Array<CourseGimmickKind> gimmicks{};

        Array<CourseStrip> midwayStrips{};

        float totalLength{};
    };

    struct CourseNode
    {
        Float3 pos{};
        int roll{}; // degrees
        int width{};
        int centerOffset{};
        CourseSegmentStyle style{};
        Array<CourseGimmickKind> gimmicks{};

        [[nodiscard]]
        float rollRadians() const;

        [[nodiscard]]
        float leftWidth() const;

        [[nodiscard]]
        float rightWidth() const;

        [[nodiscard]]
        static CourseNode Default();
    };

    struct CourseData
    {
        Array<CourseNode> nodes{};
    };

    CourseData LoadCourseData(const std::string& filepath);

    void SaveCourseData(const CourseData& course, const std::string& filepath);
}
