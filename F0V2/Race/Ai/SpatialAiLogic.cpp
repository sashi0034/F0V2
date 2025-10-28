#include "pch.h"
#include "SpatialAiLogic.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"

namespace
{
}

namespace Race
{
    SpatialData BuildSpatialData()
    {
        SpatialData state{};

        const auto& segments = GetRaceContext().stageManager().courseSegments();
        for (int s = 0; s < segments.size(); ++s)
        {
            state.segmentOffsetTable.push_back(state.waypoints.size());

            auto& segment = segments[s];
            for (int m = 0; m < segment.midwayStrips.size(); ++m)
            {
                const auto& strip = segment.midwayStrips[m];

                SpatialWaypoint waypoint;
                waypoint.indexInList = state.waypoints.size();
                waypoint.segmentIndex = s;
                waypoint.stripIndex = m;
                waypoint.position = strip.center;
                waypoint.normal = strip.normal;
                waypoint.forward = strip.toNext.normalized();
                state.waypoints.push_back(waypoint);
            }
        }

        // -----------------------------------------------
        // curveFactor

        for (int i = 0; i < state.waypoints.size(); ++i)
        {
            // 先の点の forward をサンプリングして内積を計算
            auto& waypoint = state.waypoints[i];
            const auto& w1 = state.waypoints[(i + 2) % state.waypoints.size()];
            const auto& w2 = state.waypoints[(i + 4) % state.waypoints.size()];
            const auto& w3 = state.waypoints[(i + 8) % state.waypoints.size()];

            const float dot1 = waypoint.forward.dot(w1.forward);
            const float dot2 = waypoint.forward.dot(w2.forward);
            const float dot3 = waypoint.forward.dot(w3.forward);

            // 注: これは数学的に根拠のない計算である
            waypoint.curveHeuristic =
                (1.0f - dot1 * dot1 * dot1) * 0.5f +
                (1.0f - dot2 * dot2 * dot2) * 0.5f +
                (1.0f - dot3 * dot3 * dot3) * 0.5f;
            waypoint.curveHeuristic = waypoint.curveHeuristic / 3.0f;
            // waypoint.curveHeuristic = 1.0f - Math::Square(1.0f - waypoint.curveHeuristic);
        }

        // -----------------------------------------------

        return state;
    }
}
