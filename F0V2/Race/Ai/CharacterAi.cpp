#include "pch.h"
#include "CharacterAi.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/PrimitiveModel3D.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
}

struct CharacterAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsState m_physicsState{};
    MachinePhysicsProps m_physicsProps{};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(m_physicsState.m_radius, m_physicsState.m_height, Palette::SandyBrown)
        };

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsState();
        resetPhysicsProps();
    }

private:
    void update() override
    {
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * m_physicsState.m_pose.getMatrix()).draw();

        if (m_physicsState.m_velocity.lengthSq() < Math::Square(100.0f))
        {
            m_physicsProps.input.accelPressed = true;
        }
        else
        {
            m_physicsProps.input.accelPressed = false;
        }

        UpdateMachinePhysicsState(m_physicsState, m_physicsProps);
    }

    void resetPhysicsState()
    {
        m_physicsState = {};

        m_physicsState.m_pose.position = GetRaceContext().stageManager().startPosition();

        m_physicsState.m_durability = m_physicsProps.maxDurability;
    }

    void resetPhysicsProps()
    {
        m_physicsProps.peakVelocity = 100.0f;

        m_physicsProps.accelFactor = 1.0f;
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"CharacterAi";
    }
};

namespace Race
{
    CharacterAi::CharacterAi() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CharacterAi::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> CharacterAi::asGameObject() const
    {
        return p_impl;
    }
}
