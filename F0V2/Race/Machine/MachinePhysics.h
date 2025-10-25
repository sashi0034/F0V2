#pragma once
#include "LapProgress.h"
#include "TY_Extension/Pose.h"

namespace Race
{
    struct MachinePhysicsState
    {
        float m_radius = 1;

        float m_height = 2;

        Pose m_pose{};

        Float3 m_forwardVector{0, 1, 0};

        Float3 m_upVector{0, 1, 0};

        Float3 m_velocity{};

        Float3 m_gravity{0, -1, 0};

        Float3 m_surfaceNormal{};

        Float3 m_surfaceToTriangle{};

        float m_additionalBoost{};

        LapProgress m_lapProgress{};

        Float3 rightVector() const;
    };

    struct MachinePhysicsProps
    {
        bool hasAccelInput{};

        float maxVelocity{};

        float accelerationRate{};

        bool debugPrint{};
    };

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);
}
