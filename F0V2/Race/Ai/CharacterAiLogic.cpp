#include "pch.h"
#include "CharacterAiLogic.h"

#include "SpatialAi.h"
#include "Race/IRaceContext.h"
#include "TY/GameTime.h"
#include "TY/Immediate2D.h"
#include "TY/Immediate3D.h"
#include "TY/Palette.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    // void refreshWaypointIndex(
    //     CharacterAiLogicState& state, const MachinePhysicsUnit& machine, const Array<SpatialWaypoint>& waypoints)
    // {
    //     for (;;)
    //     {
    //         if (state.m_targetWaypointIndex >= waypoints.size() - 1)
    //         {
    //             break;
    //         }
    //
    //         LapProgress targetWaypoint = machineState.m_reachedLapProgress;
    //         targetWaypoint.lapIndex = machineState.m_reachedLapProgress.lapIndex;
    //         targetWaypoint.segmentIndex = waypoints[state.m_targetWaypointIndex].segmentIndex;
    //         targetWaypoint.stripIndex = waypoints[state.m_targetWaypointIndex].stripIndex;
    //         if (machineState.m_lapProgress.isLessThan(targetWaypoint))
    //         {
    //             break;
    //         }
    //
    //         state.m_targetWaypointIndex++;
    //     }
    // }
}

namespace Race
{
    MachinePhysicsProps::input_t UpdateCharacterAiLogic(CharacterAiLogicState& state, const MachinePhysicsUnit& machine)
    {
        MachinePhysicsProps::input_t input{};

        const auto& machineState = machine.state;
        const auto& machineProps = machine.props;

        const auto& spatialData = GetRaceContext().spatialAi().data();
        const auto& currentLap = machineState.m_lapProgress;
        const auto& currentWaypoint = spatialData.takeWaypoint(currentLap.segmentIndex, currentLap.stripIndex);

        const Float3& upVector = machineState.m_upVector;

        Float3 V = machineState.m_velocity;
        V = V - upVector * upVector.dot(V);
        V = V.normalized();

        const int lookaheadCount = Min<int>(20, 10 + static_cast<int>(V.length() / 5.0f));

        ImmediatePrint("lookaheadCount: {}", lookaheadCount);

        int targetWaypointIndex = currentWaypoint.indexInList + lookaheadCount;

        // Gap 対策
        // FIXME: 簡潔にしたい
        for (;;)
        {
            targetWaypointIndex = targetWaypointIndex % spatialData.waypoints.size();
            if (spatialData.waypoints[targetWaypointIndex].targetStrip.style != CourseSegmentStyle::Gap)
            {
                break;
            }

            targetWaypointIndex++;
        }

        if (machineState.isHovering())
        {
            // 空中にいるなら更に先読みをする
            const int lookaheadCount2 = lookaheadCount * 2; // TODO
            targetWaypointIndex = (targetWaypointIndex + lookaheadCount2) % spatialData.waypoints.size();
        }

        const SpatialWaypoint& targetWaypoint = spatialData.waypoints[targetWaypointIndex];
        const Float3 toWaypoints = targetWaypoint.boundaryCenter - machineState.m_pose.position;
        const Float3 wayVector = toWaypoints.normalized();

        const float curveHeuristic = currentWaypoint.curveHeuristic;

        // accelPressed
        {
            if (machineState.isHovering())
            {
                Float3 V = machineState.m_velocity;
                // V = V - upVector * upVector.dot(V);
                V = V.normalized();
                input.accelPressed = true; // V.dot(wayVector) > 0.5f; // TODO
            }
            else if (curveHeuristic == 1.0f ||
                machineState.m_velocity.lengthSq() < Math::Square(100.0f * (1.0f - curveHeuristic)))
            {
                input.accelPressed = true;
            }
            else
            {
                input.accelPressed = false;
            }
        }

        ImmediatePrint("curveHeuristic: {:.02f}", targetWaypoint.curveHeuristic);

        // rightHandling, driftTrigger 
        {
            // wayVector は投影しないほうが安定する模様

            Float3 F = machineState.m_forwardVector;
            F = F - upVector * upVector.dot(F);
            F = F.normalized();

            Float3 V = machineState.m_velocity;
            V = V - upVector * upVector.dot(V);
            V = V.normalized();

            const float dotF = wayVector.dot(F);
            const float dotV = wayVector.dot(V);
            const bool useF = dotF < dotV;
            const float turningIntensity = 1.0f - Max(0.0f, useF ? dotF : dotV);
            ImmediatePrint("turningIntensity: {:.02f}", turningIntensity);

            if (turningIntensity > 0.01f)
            {
                const Float3 cross = wayVector.cross(useF ? F : V);
                const float rightSign = cross.dot(targetWaypoint.normal()) < 0.0f ? 1.0f : -1.0f;
                input.rightHandling = rightSign * Max(turningIntensity, 0.5f);
                input.driftTrigger = rightSign * (turningIntensity > 0.1 ? 1.0f : 0.0f);
            }

            // state.m_accumulatedRightHandling = Math::Clamp(state.m_accumulatedRightHandling, -1.0f, 1.0f);
            // input.rightHandling = state.m_accumulatedRightHandling;
        }

#if defined(_DEBUG)
        ImmediatePrint_TopRight("[AI-san]");
        ImmediatePrint_TopRight("targetWaypoint: {}", targetWaypoint.indexInList);
        ImmediatePrint_TopRight("accelPressed: {}", input.accelPressed);
        ImmediatePrint_TopRight("rightHandling: {:+.02f}", input.rightHandling);
        ImmediatePrint_TopRight("driftTrigger: {:+.02f}", input.driftTrigger);
        ImmediatePrint_TopRight("velocity: {:.01f} km/h", machineState.m_velocity.length() * 10.0f);

        {
            const Float3 n = machineState.m_upVector;
            Immediate3D::Line{machineState.m_pose.position + n, targetWaypoint.boundaryCenter + n}
                .setColor(Palette::Chartreuse)
                .pushAuto();

            Immediate3D::Line{targetWaypoint.leftBoundary + n, targetWaypoint.rightBoundary + n}
                .setColor(Palette::Aquamarine)
                .pushAuto();
        }
#endif

        return input;
    }
}
