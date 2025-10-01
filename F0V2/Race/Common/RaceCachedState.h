#pragma once
#include "CourseData.h"
#include "TY/Array.h"
#include "TY/InlineComponent.h"

namespace Race
{
    struct RaceCachedState : IInlineComponent
    {
        Array<CourseSegment> courseSegments{};
    };

    inline InlineComponent<RaceCachedState> g_cachedState{};
}
