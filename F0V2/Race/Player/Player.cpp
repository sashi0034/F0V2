#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/GameStep.h"
#include "TY/GameTime.h"
#include "TY/KeyboardInput.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"

using namespace Race;

namespace
{
    bool s_stopMove{};

    bool s_fixedCameraUp{};
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsState m_physicsState{};
    MachinePhysicsProps m_physicsProps{};

    Float3 m_cameraUp{0, 1, 0};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(m_physicsState.m_radius, m_physicsState.m_height, ColorF32{0.5f, 0.7f, 1.0f})
        };

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsState();
    }

private:
    void update() override
    {
        m_drawer.uploadWorldMatrix(m_physicsState.m_pose.getMatrix()).draw();

        const Float3 forwardVector = m_physicsState.m_forwardVector;

        Float3 eyePos = m_physicsState.m_pose.position;
        eyePos += -forwardVector.normalized() * 10.0f;
        eyePos += m_cameraUp * 5.0f;

        // for (const float dt : StandardStep_60Hz())
        // {
        //     m_cameraUp = m_cameraUp.slerp(m_physicsState.m_upVector, dt);
        // }
        m_cameraUp = m_cameraUp.slerp(m_physicsState.m_upVector, InGameDeltaTime() * 5.0f);

#ifdef _DEBUG
        if (s_fixedCameraUp) m_cameraUp = Float3{0, 1, 0};
#endif

        GetRaceContextContent().camera.set(eyePos, m_physicsState.m_pose.position, m_cameraUp);

        m_physicsProps.hasAccelInput = KeyUp.pressed();

        m_physicsProps.debug.drawHitTris = true;

        {
            float rotateInput = (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

            // TODO: 修正
            m_physicsState.m_forwardVector += m_physicsState.rightVector() * rotateInput * InGameDeltaTime();
        }

#ifdef _DEBUG
        if (s_stopMove)
        {
            auto previousState = m_physicsState;
            UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
            m_physicsState = previousState;
        }
        else
#endif
        {
            UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
        }

        ImmediateDrawer::Global().draw();

        // -----------------------------------------------

        debugUI();
    }

    void resetPhysicsState()
    {
        m_physicsState = {};

        m_physicsState.m_pose.position = GetRaceContext().stageManager().courseSegments()[0].p1 + Float3{0, 5, 0};
    }

    void debugUI()
    {
        ImGui::Begin("Player");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::DragFloat3("Position", &m_physicsState.m_pose.position.x, 0.1f);

        if (ImGui::CollapsingHeader("Checkpoint Teleport"))
        {
            static int s_checkpointIndex{};
            ImGui::InputInt("Checkpoint Index", &s_checkpointIndex);

            const auto& segments = GetRaceContext().stageManager().courseSegments();
            s_checkpointIndex = Math::Clamp<int>(s_checkpointIndex, 0, static_cast<int>(segments.size() - 1));

            if (ImGui::Button("Go To Checkpoint"))
            {
                const auto& s = segments[s_checkpointIndex];
                m_physicsState.m_pose.position = s.p1 + s.midwayStrips[0].normal * 10.0f;
                m_physicsState.m_pose.rotation = {};
            }
        }

        // -----------------------------------------------

        ImGui::Separator();

        static std::deque<MachinePhysicsState> s_physicsHistory{};
        static int s_rewindFrames{};

        if (not s_stopMove)
        {
            s_physicsHistory.push_back(m_physicsState);
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
            m_physicsState = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
        }

        if (ImGui::IsItemDeactivatedAfterEdit())
        {
            s_stopMove = false;
        }

        if (ImGui::InputInt("(Rewind Frames9", &s_rewindFrames))
        {
            s_rewindFrames = std::clamp(s_rewindFrames, 0, static_cast<int>(s_physicsHistory.size()) - 1);
            s_stopMove = true;
            m_physicsState = s_physicsHistory[s_physicsHistory.size() - 1 - s_rewindFrames];
        }

        // -----------------------------------------------

        ImGui::Checkbox("Fix Camera Up", &s_fixedCameraUp);

        const auto& surfaceNormal = m_physicsState.m_surfaceNormal;
        ImGui::Text("Normal: (%.3f, %.3f, %.3f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

        ImGui::Separator();

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::End();
    }

    // -----------------------------------------------

    void killed() override
    {
        m_children.killEach();
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
