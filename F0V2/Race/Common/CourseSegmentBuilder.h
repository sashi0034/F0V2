#pragma once
#include "CourseData.h"

namespace Race
{
    /// @return 変更した segments 要素のインデックス配列
    Array<int> BuildCourseSegmentIfNeeded(
        Array<CourseSegment>& segments,
        const Array<CourseNode>& nodeList);
}
