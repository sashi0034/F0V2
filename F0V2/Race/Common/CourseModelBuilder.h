#pragma once
#include "CourseData.h"
#include "CourseTriangleAttribute.h"
#include "TY/Array.h"
#include "TY/ModelBuffer.h"
#include "TY/PrimitiveTypes3D.h"
#include "TY/TriangleBvh.h"

namespace Race
{
    struct CoursePolygoneCollider
    {
        Array<IndexedTriangle> groundTris{};
        Array<GroundTriangleAttribute> groundAttrs{};

        Array<IndexedTriangle> gimmickTris{};
        Array<GimmickTriangleAttribute> gimmickAttrs{};
    };

    struct GimmickPlacement
    {
        GimmickTriangleAttribute kind{};
        int stripIndex{};
        Float3 left{};
        Float3 right{};
    };

    struct CourseModelBuilderOptions
    {
        bool createStartingLine{};
        CoursePolygoneCollider* outCollider = nullptr;
        Array<GimmickPlacement>* outGimmickPlacements = nullptr;
    };

    ModelBuffer BuildCourseModel(const CourseSegment& segment, const CourseModelBuilderOptions& options);
}
