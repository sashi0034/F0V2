#pragma once
#include "TY/Vector3D.h"

namespace Race
{
    struct SpatialWaypoint
    {
        Float3 pos;
        int segmentIndex{};
        int stripIndex{};
    };
}
