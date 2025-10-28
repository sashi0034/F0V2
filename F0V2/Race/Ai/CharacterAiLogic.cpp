#include "pch.h"
#include "CharacterAiLogic.h"

#include "SpatialAi.h"
#include "Race/IRaceContext.h"
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

        if (machine.state.m_velocity.lengthSq() < Math::Square(50.0f))
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

        // const Float3 toWaypoints = targetWaypoint.position - machine.state.m_pose.position;

        const Float3 r = targetWaypoint.forward.cross(machine.state.m_forwardVector);

        const bool wantsRight = r.dot(targetWaypoint.normal) < 0.0f;

        input.rightHandling = wantsRight ? 1.0f : -1.0f;

#if defined(_DEBUG)
        ImmediatePrint_TopRight("m_targetWaypointIndex: {}", state.m_targetWaypointIndex);

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
