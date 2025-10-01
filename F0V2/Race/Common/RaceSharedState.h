#pragma once
#include "CourseData.h"
#include "TY/Array.h"
#include "TY/InlineComponent.h"

namespace Race
{
    struct RaceSharedState : IInlineComponent
    {
        float groundPositionY = -50.0f;

        float fovFarZ = 1000.0f;

        Array<CourseSegment> courseSegments{};
    };

    inline InlineComponent<RaceSharedState> g_sharedState{};
}
