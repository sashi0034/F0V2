#pragma once
#include "SpatialTypes.h"
#include "TY/Array.h"

namespace Race
{
    struct SpatialAiLogicState
    {
        Array<SpatialWaypoint> nodes{};
    };

    SpatialAiLogicState BuildSpatialAiLogic();
}
