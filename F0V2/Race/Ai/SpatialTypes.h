#pragma once
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct SpatialWaypoint
    {
        int indexInList{};
        Float3 position{};
        Float3 normal{};
        Float3 forward{};
        int segmentIndex{};
        int stripIndex{};
    };

    struct SpatialData
    {
        Array<SpatialWaypoint> waypoints{};
        Array<int> segmentOffsetTable{};

        [[nodiscard]]
        const SpatialWaypoint& takeWaypoint(int segmentIndex, int stripIndex) const;
    };
}
