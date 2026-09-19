#pragma once
#include "CourseData.h"
#include "CourseModelShape.h"
#include "CourseTriangleAttribute.h"
#include "TY/Array.h"
#include "TY/PrimitiveTypes3D.h"
#include "TY/TriangleBvh.h"

namespace Race
{
    class CourseMinimapModelBuilder;

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
        CourseSegmentStyle priorStyle{};
        CourseSegmentStyle nextStyle{};
        CoursePolygoneCollider* outCollider = nullptr;
        Array<GimmickPlacement>* outGimmickPlacements = nullptr;
        CourseMinimapModelBuilder* outMinimapModel = nullptr;
    };

    CourseModelData BuildCourseModel(const CourseSegment& segment, const CourseModelBuilderOptions& options);
}
