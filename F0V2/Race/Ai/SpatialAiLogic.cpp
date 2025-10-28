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
                const auto& strip = segment.midwayStrips[m];

                const bool isFirst = state.nodes.empty();
                const bool isLast = s == segments.size() - 1 && m == segment.midwayStrips.size() - 1;

                // if (isFirst ||
                //     isLast ||
                //     (strip.center - state.nodes.back().position).lengthSq() > Math::Square(50.0f))
                {
                    SpatialWaypoint waypoint;
                    waypoint.position = strip.center;
                    waypoint.normal = strip.normal;
                    waypoint.forward = strip.toNext.normalized();
                    waypoint.segmentIndex = s;
                    waypoint.stripIndex = m;
                    state.nodes.push_back(waypoint);
                }
            }
        }

        return state;
    }
}
