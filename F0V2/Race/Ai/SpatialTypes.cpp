#include "pch.h"

#include "SpatialTypes.h"

namespace Race
{
    Float3 SpatialWaypoint::normal() const
    {
        return targetStrip.normal;
    }

    const SpatialWaypoint& SpatialData::fetchWaypoint(int segmentIndex, int stripIndex) const
    {
        return waypoints[segmentOffsetTable[segmentIndex] + stripIndex];
    }
}
