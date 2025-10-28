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
                waypoint.targetStrip = strip;
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
            if (waypoint.targetStrip.style != CourseSegmentStyle::Road)
            {
                // Road 以外は curveFactor: 0
                continue;
            }

            // 注: これは数学的に根拠のないヒューリスティックである

            static constexpr std::array offsets{4, 8, 16, 20};
            static constexpr std::array weights{0.25f, 0.25f, 0.25f, 0.25f};
            static_assert(offsets.size() == weights.size());

            float sum = 0.0f;
            for (size_t j = 0; j < offsets.size(); ++j)
            {
                const auto& w = state.waypoints[(i + offsets[j]) % state.waypoints.size()];
                const float dot = waypoint.forward.dot(w.forward);
                const float c = (1.0f - dot * dot * dot) * 0.5f;
                sum += c * weights[j];
            }

            waypoint.curveHeuristic = 1.0f - Math::Square(1.0f - sum);
            waypoint.curveHeuristic = Math::Clamp(waypoint.curveHeuristic, 0.0f, 1.0f);
        }

        // -----------------------------------------------

        return state;
    }
}
