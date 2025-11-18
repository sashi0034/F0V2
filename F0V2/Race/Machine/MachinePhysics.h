#pragma once
#include "LapProgress.h"
#include "MachineConstants.h"
#include "TY/Color.h"
#include "TY_Extension/Pose.h"
#include "Util/PositiveValue.h"

namespace Race
{
    struct MachinePhysicsState
    {
        bool m_isRunningEventProcess{};

        float m_radius = MachineRadius;

        float m_height = MachineHeight;

        Pose m_pose{};

        Float3 m_forwardVector{0, 1, 0};

        Float3 m_visualForwardVector{0, 1, 0};

        Float3 m_upVector{0, 1, 0};

        Float3 m_velocity{};

        Float3 m_gravity{0, -1, 0};

        Float3 m_surfaceNormal{};

        Float3 m_surfaceToTriangle{};

        float m_driftOffset{};

        float m_manualBoost{};

        float m_passiveBoost{};

        PositiveF32 m_durability{};

        LapProgress m_lapProgress{};

        LapProgress m_reachedLapProgress{};

        bool m_isFallingOffCourse{};

        SegmentAndStrip m_lastGroundContactLocation{};

        [[nodiscard]]
        Float3 rightVector() const;

        [[nodiscard]]
        bool isHovering() const;

        [[nodiscard]]
        bool isHitDetectionEnabled() const;

        [[nodiscard]]
        bool isBoostUnlocked() const;

        [[nodiscard]]
        bool isDead() const;
    };

    using MachineId = int;

    struct MachinePhysicsProps
    {
        MachineId machineId{};

        ColorF32 themeColor{};

        struct input_t
        {
            bool accelPressed{};

            bool boostRequested{};

            float rightHandling{}; // [-1.0f, 1.0f]

            float driftTrigger{}; // [-1.0f, 1.0f]

            float cheatBoostFactor{1.0f};
        } input{};

        PositiveF32 maxDurability{5000.0f};

        float peakVelocity{};

        float accelFactor{};
    };

    void SetupMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);
}
