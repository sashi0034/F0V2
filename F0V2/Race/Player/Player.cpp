#include "pch.h"
#include "Player.h"

#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/RaceSharedState.h"
#include "Race/Machine/MachineDrawer.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/KeyboardInput.h"
#include "TY/Mouse.h"
#include "TY/Palette.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/SimpleInput.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_stopMove{};
#endif
}

struct Player::Impl : GameObjectBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
    ActorContainer m_children{};

    MachineDrawer m_drawer{};

    Float3 m_cameraUp{0, 1, 0};

    MachineId m_machineId{PlayerMachineId};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        m_drawer = MachineDrawer(Palette::CornflowerBlue.sRGBToLinear());

        resetPhysicsProps();
        resetPhysicsState();

        // TODO: Update 前の最初のフレームでカメラが設定されるようにする
    }

private:
    MachinePhysicsUnit& machine() const
    {
        return GetRaceContext().machineManager().fetchMachine(PlayerMachineId);
    }

    void computeEyeAndTarget(Float3& outEye, Float3& outTarget) const
    {
        const Float3 forwardVector = machine().state.m_forwardVector;

        outTarget = machine().state.m_pose.position + machine().state.m_upVector * 5.0f;

        constexpr float cameraBackward = 10.0f;
        constexpr float cameraHeight = 5.0f;

        const Float3 optimalEyePos =
            outTarget - forwardVector.normalized() * cameraBackward + m_cameraUp * cameraHeight;

        const auto ray = LineSegment3D{outTarget, optimalEyePos};
        const auto hit =
            GetRaceContext().stageManager().stageStaticCollider().rayCastGround(ray);
        if (hit.has_value())
        {
            // 地面にカメラが遮られているなら、その面に垂線の足をおろしてカメラ位置とする
            const Float3 H = hit->triangle.asPlane().projection(optimalEyePos);
            outEye = H;
            return;
        }

        outEye = optimalEyePos;
    }

    void update() override
    {
        // 前フレームの camera & 前フレームの Player 描画方式
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * machine().state.m_pose.getMatrix());

        updatePhysics();

        debugUI();
    }

    void drawShadowMap() const override
    {
        m_drawer.drawShadowMap();
    }

    void drawGBuffer() const override
    {
        m_drawer.drawGBuffer();
    }

    void resetPhysicsState()
    {
        SetupMachinePhysicsState(machine().state, machine().props);
    }

    void resetPhysicsProps()
    {
        machine().props.machineId = 0;

        machine().props.peakVelocity = 100.0f;

        machine().props.accelFactor = 1.0f;
    }

    void updatePhysics()
    {
        MachinePhysicsProps::input_t input;

        input.accelPressed = KeyLShift.pressed();

        input.boostRequested = KeySpace.down();

        input.rightHandling =
            (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

        input.driftTrigger =
            (KeyLeft.pressed() ? -1.0f : (KeyRight.pressed() ? 1.0f : 0.0f));

#if defined(_DEBUG)
        if (g_debugService.disablePlayerInput)
        {
            input = {};
        }
#endif

        machine().props.input = input;

#if defined(_DEBUG)
        if (s_stopMove)
        {
            auto previousState = machine().state;
            UpdateMachinePhysicsState(machine().state, machine().props);
            machine().state = previousState;
        }
        else
#endif
        {
            UpdateMachinePhysicsState(machine().state, machine().props);
        }
    }

    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("Player");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::DragFloat3("Position", &machine().state.m_pose.position.x, 0.1f);

        if (ImGui::CollapsingHeader("Checkpoint Teleport"))
        {
            static int s_checkpointIndex{};
            ImGui::InputInt("Checkpoint Index", &s_checkpointIndex);

            const auto& segments = GetRaceContext().stageManager().courseSegments();
            s_checkpointIndex = Math::Clamp<int>(s_checkpointIndex, 0, static_cast<int>(segments.size() - 1));

            if (ImGui::Button("Go To Checkpoint"))
            {
                const auto& s = segments[s_checkpointIndex];
                machine().state = {};
                machine().state.m_pose.position = s.p1 + s.midwayStrips[0].normal * 10.0f;

                s_stopMove = false;
            }
        }

        // -----------------------------------------------
        ImGui::Separator();

        static std::deque<MachinePhysicsState> s_physicsHistory{};
        static int s_rewindFrames{};

        if (not s_stopMove)
        {
            s_physicsHistory.push_back(machine().state);
            s_rewindFrames = 0;
        }

        while (s_physicsHistory.size() > 300)
        {
            s_physicsHistory.pop_front();
        }

        if (ImGui::SliderInt("Rewind Frames", &s_rewindFrames, static_cast<int>(s_physicsHistory.size()) - 1, 0))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            machine().state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            machine().state.m_velocity = {};
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            s_stopMove = false;
        }

        if (ImGui::InputInt("(Rewind Frames)", &s_rewindFrames))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            machine().state = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
            machine().state.m_velocity = {};
        }

        // -----------------------------------------------

        const auto& surfaceNormal = machine().state.m_surfaceNormal;
        ImGui::Text("Normal: (%.3f, %.3f, %.3f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

        ImGui::Separator();

        ImGui::DragFloat("Max Velocity", &machine().props.peakVelocity);

        ImGui::DragFloat("Acceleration Rate", &machine().props.accelFactor, 0.01f);

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::End();
#endif
    }

    void killed() override
    {
        m_children.killEach();

        GetRaceContext().unregisterDrawer(this);
    }

    std::u32string name() const override
    {
        return U"Player";
    }
};

namespace Race
{
    Player::Player() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void Player::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> Player::asGameObject() const
    {
        return p_impl;
    }
}
