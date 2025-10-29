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

        // -----------------------------------------------
        // curveFactor

        for (int i = 0; i < state.waypoints.size(); ++i)
        {
            // 先の点の forward をサンプリングして内積を計算
            auto& waypoint = state.waypoints[i];
            if (waypoint.targetStrip.style != CourseSegmentStyle::Road)
            {
                // Road 以外
                waypoint.leftBoundary = waypoint.targetStrip.center;
                waypoint.rightBoundary = waypoint.targetStrip.center;
                waypoint.boundaryCenter = waypoint.targetStrip.center;
                continue;
            }

            // 注: これは数学的に根拠のないヒューリスティックである

            static constexpr std::array offsets{4, 8, 16, 20};
            static constexpr std::array weights{0.25f, 0.25f, 0.25f, 0.25f};
            static_assert(offsets.size() == weights.size());

            // curveHeuristic
            {
                float sum = 0.0f; // [0, 1]
                for (int n = 0; n < offsets.size(); ++n)
                {
                    const auto& w = state.waypoints[(i + offsets[n]) % state.waypoints.size()];
                    const float dot = waypoint.forward.dot(w.forward);
                    const float c = (1.0f - Math::Pow5(dot)) * 0.5f;
                    sum += c * weights[n];
                }

                waypoint.curveHeuristic = 1.0f - Math::Square(1.0f - sum);
                waypoint.curveHeuristic = Math::Clamp(waypoint.curveHeuristic, 0.0f, 1.0f);
            }

            // leftBoundary, rightBoundary
            {
                float sum{}; // [-1, 1]
                for (int n = 0; n < offsets.size(); ++n)
                {
                    const auto& w = state.waypoints[(i + offsets[n]) % state.waypoints.size()];
                    const float dot = waypoint.right.dot(w.forward);
                    const float c = dot;
                    sum += c * weights[n];
                }

                if (sum > 0.0f)
                {
                    waypoint.leftBoundary =
                        waypoint.targetStrip.leftmost + waypoint.right * waypoint.targetStrip.width * sum;
                    waypoint.rightBoundary =
                        waypoint.targetStrip.rightmost;
                }
                else // sum <= 0.0f
                {
                    waypoint.leftBoundary =
                        waypoint.targetStrip.leftmost;
                    waypoint.rightBoundary =
                        waypoint.targetStrip.rightmost + waypoint.right * waypoint.targetStrip.width * sum;
                }

                waypoint.boundaryCenter = (waypoint.leftBoundary + waypoint.rightBoundary) * 0.5f;
            }
        }

        // -----------------------------------------------

        return state;
    }
}
