#pragma once
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct SpatialWaypoint
    {
        int indexInList{};
        int segmentIndex{};
        int stripIndex{};
        Float3 position{};
        Float3 normal{};
        Float3 forward{};
        float curveHeuristic{}; // [0.0, 1.0]
    };

    struct SpatialData
    {
        Array<SpatialWaypoint> waypoints{};
        Array<int> segmentOffsetTable{};

        [[nodiscard]]
        const SpatialWaypoint& takeWaypoint(int segmentIndex, int stripIndex) const;
    };
}
