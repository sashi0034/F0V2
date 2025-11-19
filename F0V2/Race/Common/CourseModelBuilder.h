#pragma once
#include "CourseData.h"
#include "CourseTriangleAttribute.h"
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

    struct CourseModelBuilderOptions
    {
        bool createStartingLine{};
        CoursePolygoneCollider* outCollider = nullptr;
    };

    ModelBuffer BuildCourseModel(const CourseSegment& segment, const CourseModelBuilderOptions& options);
}
