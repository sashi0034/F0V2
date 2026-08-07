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
        float curveHeuristic{}; // [0.0, 1.0] // TODO: Remove this

        struct GimmickData
        {
            GimmickTriangleAttribute kind{};
            Float3 left{};
            Float3 right{};
            Float3 center{};
        };

        Array<GimmickData> containingGimmicks{};
        int nextGimmickWaypointIndex{-1}; // containingGimmicks が存在する次の Waypoint

        [[nodiscard]]
        Float3 normal() const;
    };

    struct SpatialData
    {
        Array<SpatialWaypoint> waypoints{};
        Array<int> segmentOffsetTable{};

        [[nodiscard]]
        const SpatialWaypoint& fetchWaypoint(int segmentIndex, int stripIndex) const;
    };
}
