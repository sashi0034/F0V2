#pragma once
#include "CourseData.h"
#include "CourseModelBuilder.h"
#include "TY/ModelData.h"

namespace Race
{
    void BuildGimmickModel(ModelData& model, const CourseSegment& segment, const CourseModelBuilderOptions& options);
}
