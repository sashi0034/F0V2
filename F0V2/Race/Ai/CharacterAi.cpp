#include "pch.h"
#include "CharacterAi.h"

#include "Asset.generated.h"
#include "CharacterAiLogic.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Machine/MachinePhysics.h"
#include "Race/Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/ImmediateDrawer.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/PrimitiveModel3D.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_stopMove{};
#endif
}

struct CharacterAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    MachinePhysicsUnit m_machine{};

    CharacterAiLogicState m_logicState{};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(m_machine.state.m_radius, m_machine.state.m_height, Palette::SandyBrown)
        };

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsState();
        resetPhysicsProps();

#if defined(_DEBUG) && 1
        g_debugService.monitorMachineId = 1;
#endif
    }

private:
    void update() override
    {
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * m_machine.state.m_pose.getMatrix()).draw();

        m_machine.props.input = UpdateCharacterAiLogic(m_logicState, m_machine);

#if defined(_DEBUG)
        if (not s_stopMove)
#endif
        {
            UpdateMachinePhysicsState(m_machine.state, m_machine.props);
        }

        ImmediateDrawer::Global().draw();

        debugUI();
    }

    void resetPhysicsState()
    {
        m_machine.state = {};

        m_machine.state.m_pose.position = GetRaceContext().stageManager().startPosition();

        m_machine.state.m_forwardVector = GetRaceContext().stageManager().courseSegments()[0].midwayStrips[0].toNext;

        m_machine.state.m_durability = m_machine.props.maxDurability;
    }

    void resetPhysicsProps()
    {
        m_machine.props.machineId = 1; // TODO

        m_machine.props.peakVelocity = 100.0f;

        m_machine.props.accelFactor = 1.0f;
    }

    void debugUI()
    {
        ImGui::Begin("Character AI");

        ImGui::Checkbox("Stop Move", &s_stopMove);

        if (ImGui::DragFloat3("Position", &m_machine.state.m_pose.position.x))
        {
            s_stopMove = true;
        }

        ImGui::Separator();

        if (ImGui::Button("Reset"))
        {
            resetPhysicsState();
            resetPhysicsProps();
            m_logicState = {};
        }

        ImGui::End();
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

    const MachinePhysicsUnit& CharacterAi::machine() const
    {
        return p_impl->m_machine;
    }

    std::shared_ptr<GameObjectBase> CharacterAi::asGameObject() const
    {
        return p_impl;
    }
}
