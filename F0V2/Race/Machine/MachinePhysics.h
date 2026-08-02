#pragma once
#include "LapProgress.h"
#include "MachineConstants.h"
#include "Race/Common/CourseGimmick.h"
#include "TY/Color.h"
#include "TY_Extension/Pose.h"
#include "Util/PositiveValue.h"

namespace Race
{
    struct MachinePhysicsState
    {
        bool m_isRunningEventProcess{};

        float m_radius = MachineRadius;

        float m_cylinderLength = MachineCylinderLength;

        Pose m_pose{};

        Float3 m_forwardVector{0, 1, 0};

        Float3 m_visualForwardVector{0, 1, 0};

        Float3 m_upVector{0, 1, 0};

        Float3 m_velocity{};

        Float3 m_gravity{0, -1, 0};

        Float3 m_surfaceNormal{};

        Float3 m_surfaceToTriangle{};

        float m_driftOffset{};

        float m_rawPitchRate{};

        float m_pitchRate{};

        float m_impulseTurn{}; // neutral: 0.0f

        float m_impulseTurnTime{};

        bool m_stabilizingAfterImpulseTurn{};

        float m_manualBoost{};

        float m_manualBoostCooldownTime{};

        float m_boostComboCountdown{};

        int m_boostComboCount{};

        float m_passiveBoost{};

        PositiveF32 m_durability{};

        LapProgress m_lapProgress{};

        LapProgress m_markedLapProgress{};

        bool m_isFallingOffCourse{};

        SegmentAndStrip m_lastGroundContactLocation{};

        GimmickFlagBits m_previousTouchingGimmicks{};

        GimmickFlagBits m_touchingGimmicks{};

        float m_lastAttackedByOtherMachineTime{};

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

            float pitch{}; // [-1.0f, 1.0f]: negative = pitch up, positive = pitch down

            float driftTrigger{}; // [-1.0f, 1.0f]

            bool impulseTurnRequested{};

            float cheatBoostFactor{1.0f};
        } input{};

        PositiveF32 maxDurability{5000.0f};

        PositiveF32 boostCost{800.0f};

        float peakVelocity{};

        float accelFactor{};
    };

    void SetupMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);

    struct MachinePhysicsUpdateOutcome
    {
        bool accelInputAccepted{};
        bool driftInputAccepted{};
        bool boostInputAccepted{};
    };

    MachinePhysicsUpdateOutcome UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);
}
