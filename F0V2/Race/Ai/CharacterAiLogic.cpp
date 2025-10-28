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
    void refreshWaypointIndex(
        CharacterAiLogicState& state, const MachinePhysicsUnit& machine, const Array<SpatialWaypoint>& waypoints)
    {
        for (;;)
        {
            if (state.m_targetWaypointIndex >= waypoints.size() - 1)
            {
                break;
            }

            LapProgress targetWaypoint = machine.state.m_reachedLapProgress;
            targetWaypoint.lapIndex = machine.state.m_reachedLapProgress.lapIndex;
            targetWaypoint.segmentIndex = waypoints[state.m_targetWaypointIndex].segmentIndex;
            targetWaypoint.stripIndex = waypoints[state.m_targetWaypointIndex].stripIndex;
            if (machine.state.m_lapProgress.isLessThan(targetWaypoint))
            {
                break;
            }

            state.m_targetWaypointIndex++;
        }
    }
}

namespace Race
{
    MachinePhysicsProps::input_t UpdateCharacterAiLogic(CharacterAiLogicState& state, const MachinePhysicsUnit& machine)
    {
        MachinePhysicsProps::input_t input{};

        // -----------------------------------------------

        if (machine.state.m_velocity.lengthSq() < Math::Square(20.0f))
        {
            input.accelPressed = true;
        }
        else
        {
            input.accelPressed = false;
        }

        // -----------------------------------------------

        if (state.m_lastLapIndex < machine.state.m_reachedLapProgress.lapIndex)
        {
            state.m_targetWaypointIndex = 0;
            state.m_lastLapIndex = machine.state.m_reachedLapProgress.lapIndex;
        }

        const auto& spatialAi = GetRaceContext().spatialAi();
        const auto& waypoints = spatialAi.waypoints();

        refreshWaypointIndex(state, machine, waypoints);

        // -----------------------------------------------

        const SpatialWaypoint& targetWaypoint = waypoints[state.m_targetWaypointIndex];

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
            const Float3 f =
                toWaypoints.normalized().normalized();
            // targetWaypoint.forward.normalized();
            // toWaypoints.normalized().normalized();
            const Float3 machineForward =
                machine.state.m_velocity.normalized();
            // machine.state.m_forwardVector;
            const float cosTheta = machineForward.dot(f);
            const float v = 1.0f - std::abs(cosTheta);
            ImmediatePrint("v: {:.02f}", v);

            {
                const Float3 cross = machineForward.cross(f);
                if (cross.dot(targetWaypoint.normal) < 0.0f)
                {
                    state.m_accumulatedRightHandling -= v * 100.0f * InGameDeltaTime();
                    // input.rightHandling = -v;
                    input.driftTrigger = v > 0.1 ? -1.0f : 0.0f;
                }
                else
                {
                    state.m_accumulatedRightHandling += v * 100.0f * InGameDeltaTime();
                    // input.rightHandling = v;
                    input.driftTrigger = v > 0.1 ? 1.0f : 0.0f;
                }
            }

            state.m_accumulatedRightHandling = Math::Clamp(state.m_accumulatedRightHandling, -1.0f, 1.0f);
            input.rightHandling = state.m_accumulatedRightHandling;
        }

#if defined(_DEBUG)
        ImmediatePrint_TopRight("m_targetWaypointIndex: {}", state.m_targetWaypointIndex);
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
