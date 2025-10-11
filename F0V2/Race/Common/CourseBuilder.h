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
        Array<IndexedTriangle> tris{};
        Array<CourseTriangleAttribute> attributes{};
    };

    ModelBuffer BuildCourseModel(const CourseSegment& segment, CoursePolygoneCollider* outCollider = nullptr);
}
