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

        const auto& spatialAi = GetRaceContext().spatialAi();
        const auto& waypoints = spatialAi.waypoints();

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

        const SpatialWaypoint& targetWaypoint = waypoints[state.m_targetWaypointIndex];

        const Float3 toWaypoints = targetWaypoint.position - machine.state.m_pose.position;

        const Float3 r = toWaypoints.cross(machine.state.m_forwardVector);

        const bool wantsRight = r.dot(targetWaypoint.normal) < 0.0f;

        input.rightHandling = wantsRight ? 1.0f : -1.0f;

#if defined(_DEBUG)
        ImmediatePrint_TopRight("toWaypoints: {}", toWaypoints);
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
