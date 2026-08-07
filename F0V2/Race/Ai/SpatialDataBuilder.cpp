#include "pch.h"
#include "SpatialDataBuilder.h"

#include "Race/IRaceContext.h"
#include "Race/Stage/StageManager.h"

namespace
{
    using namespace Race;

    constexpr float RoadBoundaryMargin = 5.0f;

    bool containsGimmick(
        const SpatialWaypoint& waypoint,
        GimmickTriangleAttribute::kind_t kind)
    {
        return std::ranges::any_of(
            waypoint.containingGimmicks,
            [kind](const SpatialWaypoint::GimmickData& gimmick)
            {
                return gimmick.kind.kind == kind;
            });
    }

    void setupNextGimmickWaypointIndices(
        SpatialData& state,
        GimmickTriangleAttribute::kind_t kind,
        int SpatialWaypoint::* nextWaypointIndexMember /* pointer to member */)
    {
        int nextGimmickWaypointIndex = -1;
        for (int i = 0; i < state.waypoints.size(); ++i)
        {
            if (containsGimmick(state.waypoints[i], kind))
            {
                nextGimmickWaypointIndex = i;
                break;
            }
        }

        for (int i = static_cast<int>(state.waypoints.size()) - 1; i >= 0; --i)
        {
            auto& waypoint = state.waypoints[i];
            waypoint.*nextWaypointIndexMember = nextGimmickWaypointIndex;

            if (containsGimmick(waypoint, kind))
            {
                nextGimmickWaypointIndex = i;
            }
        }
    }
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
                if (strip.style == CourseSegmentStyle::Road)
                {
                    assert((strip.rightmost - strip.leftmost).length() > RoadBoundaryMargin * 2.0f);
                    waypoint.leftBoundaryWithMargin = strip.leftmost + waypoint.right * RoadBoundaryMargin;
                    waypoint.rightBoundaryWithMargin = strip.rightmost - waypoint.right * RoadBoundaryMargin;
                }

                state.waypoints.push_back(waypoint);
            }
        }

        const auto& gimmickPlacements = GetRaceContext().stageManager().gimmickPlacements();
        for (int segmentIndex = 0; segmentIndex < gimmickPlacements.size(); ++segmentIndex)
        {
            for (const auto& placement : gimmickPlacements[segmentIndex])
            {
                if (placement.kind.kind == GimmickTriangleAttribute::kind_t::Barrier)
                {
                    continue;
                }

                auto& waypoint = state.waypoints[
                    state.segmentOffsetTable[segmentIndex] + placement.stripIndex];
                waypoint.containingGimmicks.push_back(SpatialWaypoint::GimmickData{
                    .kind = placement.kind,
                    .left = placement.left,
                    .right = placement.right,
                    .center = (placement.left + placement.right) * 0.5f,
                });
            }
        }

        setupNextGimmickWaypointIndices(
            state,
            GimmickTriangleAttribute::kind_t::BoostPad,
            &SpatialWaypoint::nextBoostPadWaypointIndex);
        setupNextGimmickWaypointIndices(
            state,
            GimmickTriangleAttribute::kind_t::PitZone,
            &SpatialWaypoint::nextPitZoneWaypointIndex);

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
