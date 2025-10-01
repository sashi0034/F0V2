#pragma once
#include "CourseData.h"
#include "TY/ModelBuffer.h"

namespace Race
{
    ModelBuffer BuildCourseModel(const CourseSegment& segment);

    void DebugDrawCourse(const Array<CourseSegment>& segments);
}
