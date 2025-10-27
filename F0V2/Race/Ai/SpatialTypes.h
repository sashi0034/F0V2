#pragma once
#include "TY/Vector3D.h"

namespace Race
{
    struct SpatialWaypoint
    {
        Float3 position;
        Float3 normal{};
        int segmentIndex{};
        int stripIndex{};
    };
}
