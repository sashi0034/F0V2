#pragma once
#include "TY_Extension/Pose.h"

namespace Race
{
    struct MachinePhysicsState
    {
        float m_radius = 1;

        float m_height = 2;

        Pose m_pose{};

        float m_yaw{};

        Float3 m_velocity{};

        Float3 m_gravity{0, -1, 0};

        Float3 m_surfaceNormal{};

        float m_groundedness{}; // 接触したポリゴンの法線と上ベクトルのコサイン類似度

        Float3 m_upVector{0, 1, 0};
    };

    struct MachinePhysicsProps
    {
        bool hasAccelInput{};

        struct
        {
            bool drawHitTris{};
        } debug;
    };

    void UpdateMachinePhysicsState(MachinePhysicsState& state, const MachinePhysicsProps& props);
}
