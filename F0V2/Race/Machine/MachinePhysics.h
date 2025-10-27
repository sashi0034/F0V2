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

        float m_driftOffset{};

        float m_manualBoost{};

        float m_passiveBoost{};

        float m_durability{};

        LapProgress m_lapProgress{};

        Float3 rightVector() const;
    };

    struct MachinePhysicsProps
    {
        int machineId{};

        struct
        {
            bool accelPressed{};

            bool boostRequested{};

            float rightHandling{}; // [-1.0f, 1.0f]

            int driftTrigger{}; // -1, 0, 1
        } input{};

        float maxDurability{5000.0f};

        float peakVelocity{};

        float accelFactor{};
    };

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);
}
