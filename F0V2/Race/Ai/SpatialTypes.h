#pragma once
#include "Race/Common/CourseData.h"
#include "TY/Array.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct SpatialWaypoint
    {
        int indexInList{};
        int segmentIndex{};
        int stripIndex{};
        CourseStrip targetStrip{};
        Float3 forward{}; // normalized
        Float3 right{}; // normalized
        float curveHeuristic{}; // [0.0, 1.0]
        Float3 leftBoundary{}; // TODO: Remove this?
        Float3 rightBoundary{}; // TODO: Remove this?
        Float3 boundaryCenter{};

        [[nodiscard]]
        Float3 normal() const;
    };

    struct SpatialData
    {
        Array<SpatialWaypoint> waypoints{};
        Array<int> segmentOffsetTable{};

        [[nodiscard]]
        const SpatialWaypoint& takeWaypoint(int segmentIndex, int stripIndex) const;
    };
}
