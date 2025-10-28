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
    //         LapProgress targetWaypoint = machine.state.m_reachedLapProgress;
    //         targetWaypoint.lapIndex = machine.state.m_reachedLapProgress.lapIndex;
    //         targetWaypoint.segmentIndex = waypoints[state.m_targetWaypointIndex].segmentIndex;
    //         targetWaypoint.stripIndex = waypoints[state.m_targetWaypointIndex].stripIndex;
    //         if (machine.state.m_lapProgress.isLessThan(targetWaypoint))
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

        const auto& spatialData = GetRaceContext().spatialAi().data();
        const auto& currentLap = machine.state.m_lapProgress;
        const auto& currentWaypoint = spatialData.takeWaypoint(currentLap.segmentIndex, currentLap.stripIndex);
        constexpr int lookaheadCount = 10;
        const SpatialWaypoint& targetWaypoint =
            spatialData.waypoints[(currentWaypoint.indexInList + lookaheadCount) % spatialData.waypoints.size()];

        const float curveHeuristic = currentWaypoint.curveHeuristic;
        if (machine.state.m_velocity.lengthSq() < Math::Square(50.0f * (1.0f - curveHeuristic)))
        {
            input.accelPressed = true;
        }
        else
        {
            input.accelPressed = false;
        }

        ImmediatePrint("curveHeuristic: {:.02f}", targetWaypoint.curveHeuristic);

        const Float3 toWaypoints = targetWaypoint.position - machine.state.m_pose.position;

        // {
        //     const Float3 forward = machine.state.m_velocity.normalized();
        //     const Float3 r = toWaypoints.cross(forward).normalized();
        //
        //     const float cosTheta = r.dot(targetWaypoint.normal);
        //
        //     input.rightHandling = -cosTheta;
        // }

        {
            const Float3 wayDir = toWaypoints.normalized();
            const Float3 F = machine.state.m_forwardVector;
            const Float3 V = machine.state.m_velocity.normalized();
            const float dotF = wayDir.dot(F);
            const float dotV = wayDir.dot(V);
            const bool useF = dotF < dotV;
            const float turningIntensity = 1.0f - Max(0.0f, useF ? dotF : dotV);
            ImmediatePrint("turningIntensity: {:.02f}", turningIntensity);

            if (turningIntensity > 0.01f)
            {
                const Float3 cross = wayDir.cross(useF ? F : V);
                const float rightSign = cross.dot(targetWaypoint.normal) < 0.0f ? 1.0f : -1.0f;
                input.rightHandling = rightSign * Max(turningIntensity, 0.5f);
                input.driftTrigger = rightSign * (turningIntensity > 0.1 ? 1.0f : 0.0f);
            }

            // state.m_accumulatedRightHandling = Math::Clamp(state.m_accumulatedRightHandling, -1.0f, 1.0f);
            // input.rightHandling = state.m_accumulatedRightHandling;
        }

#if defined(_DEBUG)
        ImmediatePrint_TopRight("targetWaypoint: {}", targetWaypoint.indexInList);
        ImmediatePrint_TopRight("rightHandling: {:+.02f}", input.rightHandling);
        ImmediatePrint_TopRight("driftTrigger: {:+.02f}", input.driftTrigger);

        {
            const Float3 n = machine.state.m_upVector;
            Immediate3D::Line{machine.state.m_pose.position + n, targetWaypoint.position + n}
                .setColor(Palette::Chartreuse)
                .pushAuto();
        }
#endif

        return input;
    }
}
