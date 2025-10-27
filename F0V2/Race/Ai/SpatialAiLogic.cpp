#include "pch.h"
#include "SpatialAiLogic.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"

namespace
{
}

namespace Race
{
    SpatialAiLogicState BuildSpatialAiLogic()
    {
        SpatialAiLogicState state{};

        const auto& segments = GetRaceContext().stageManager().courseSegments();
        for (int s = 0; s < segments.size(); ++s)
        {
            auto& segment = segments[s];
            for (int m = 0; m < segment.midwayStrips.size(); ++m)
            {
                const auto nextPos = segment.midwayStrips[m].center;;
                if (state.nodes.empty() || (nextPos - state.nodes.back().pos).lengthSq() > Math::Square(10.0f))
                {
                    SpatialWaypoint waypoint;
                    waypoint.pos = nextPos;
                    waypoint.segmentIndex = s;
                    waypoint.stripIndex = m;
                    state.nodes.push_back(waypoint);
                }
            }
        }

        return state;
    }
}
