#pragma once
#include "CourseData.h"
#include "TY/ModelBuffer.h"
#include "TY/PrimitiveTypes3D.h"

namespace Race
{
    ModelBuffer BuildCourseModel(const CourseSegment& segment, Array<Triangle3D>* outCollider = nullptr);
}
