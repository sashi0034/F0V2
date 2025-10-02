#include "pch.h"
#include "Player.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/GameTime.h"
#include "TY/Intersects3D.h"
#include "TY/KeyboardInput.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ShapeDrawer.h"
#include "TY/detail/EngineKeyboardMouse.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/Pose.h"

using namespace Race;

namespace
{
    bool s_stopMove{};
}

struct Player::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsState m_physicsState{};
    MachinePhysicsProps m_physicsProps{};

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

        float rotateInput = (KeyA.pressed() ? -1.0f : 0.0f) + (KeyD.pressed() ? 1.0f : 0.0f);

        // TODO: 修正
        m_physicsState.m_yaw += rotateInput * Math::ToRadians(90.0f) * InGameDeltaTime();

        const Float3 forwardVector = m_physicsState.m_pose.rotation.rotate(Float3{0, 0, 1});

        GetRaceContextContent().camera.setEyeAndTarget(
            m_physicsState.m_pose.position - forwardVector.normalized() * 10.0f + m_physicsState.m_surfaceNormal * 5.0f,
            m_physicsState.m_pose.position);

        m_physicsProps.hasAccelInput = KeyUp.pressed();

        m_physicsProps.debug.drawHitTris = true;

        if (not s_stopMove)
        {
            UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
        }

        ShapeDrawer::Global().draw();

        // -----------------------------------------------

        debugUI();
    }

    void resetPhysicsState()
    {
        m_physicsState = {};

        m_physicsState.m_pose.position = Float3{0, 50.0f, 0};
        m_physicsState.m_pose.rotation = Quaternion::Identity();

        m_physicsState.m_surfaceNormal = Float3{0, 1, 0};
    }

    void debugUI()
    {
        ImGui::Begin("Player");

        if (ImGui::Button("Reset Physics State"))
        {
            resetPhysicsState();
        }

        ImGui::Checkbox("Stop Move", &s_stopMove);

        const auto& surfaceNormal = m_physicsState.m_surfaceNormal;
        ImGui::Text("Normal: (%.2f, %.2f, %.2f)", surfaceNormal.x, surfaceNormal.y, surfaceNormal.z);

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
