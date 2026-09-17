#pragma once
#include "CourseData.h"
#include "CourseModelBuilder.h"

namespace Race
{
    void BuildGimmickModel(
        CourseModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options);
}
