#include "pch.h"
#include "SpatialDataBuilder.h"

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
                waypoint.right = (strip.rightmost - strip.leftmost).normalized();
                state.waypoints.push_back(waypoint);
            }
        }

        const auto& gimmickPlacements = GetRaceContext().stageManager().gimmickPlacements();
        for (int segmentIndex = 0; segmentIndex < gimmickPlacements.size(); ++segmentIndex)
        {
            for (const auto& placement : gimmickPlacements[segmentIndex])
            {
                auto& waypoint = state.waypoints[
                    state.segmentOffsetTable[segmentIndex] + placement.stripIndex];
                waypoint.gimmicks.push_back(SpatialWaypoint::GimmickData{
                    .kind = placement.kind,
                    .left = placement.left,
                    .right = placement.right,
                    .center = (placement.left + placement.right) * 0.5f,
                });
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
                // Road 以外
                continue;
            }

            // 注: これは数学的に根拠のないヒューリスティックである

            // static constexpr std::array offsets{4, 8, 16, 20};
            // static constexpr std::array weights{0.25f, 0.25f, 0.25f, 0.25f};
            // static_assert(offsets.size() == weights.size());

            // curveHeuristic
            {
                float acc{}; // [0, 1]
                constexpr int heuristicStart = 4;
                constexpr int heuristicEnd = 23;
                for (int h = heuristicStart; h < heuristicEnd; ++h)
                {
                    const auto& w = state.waypoints[(i + h) % state.waypoints.size()];
                    const float dot = waypoint.forward.dot(w.forward);
                    const float c = (1.0f - Math::Pow5(dot)) * 0.5f;
                    acc += c;
                }

                acc = acc / (heuristicEnd - heuristicStart + 1);

                waypoint.curveHeuristic = 1.0f - Math::Square(1.0f - acc);
                waypoint.curveHeuristic = Math::Clamp(waypoint.curveHeuristic, 0.0f, 1.0f);
            }
        }

        // -----------------------------------------------

        return state;
    }
}
