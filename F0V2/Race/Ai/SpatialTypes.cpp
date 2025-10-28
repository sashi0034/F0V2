#include "pch.h"

#include "SpatialTypes.h"

namespace Race
{
    const SpatialWaypoint& SpatialData::takeWaypoint(int segmentIndex, int stripIndex) const
    {
        return waypoints[segmentOffsetTable[segmentIndex] + stripIndex];
    }
}
