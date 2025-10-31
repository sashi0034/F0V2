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
    bool s_stopInput{};
    bool s_stopMove{};
#endif
}

struct CharacterAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    CharacterAiLogicState m_logicState{};

    void Init()
    {
        ModelBuffer model = ModelBuffer{
            PrimitiveModel3D::Capsule(machine().state.m_radius, machine().state.m_height, Palette::SandyBrown)
        };

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsState();
        resetPhysicsProps();

#if defined(_DEBUG) && 0
        g_debugService.monitorMachineId = 1;
#endif
    }

private:
    static MachinePhysicsUnit& machine()
    {
        return GetRaceContext().machineManager().fetchMachine(1);
    }

    void update() override
    {
        static Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        m_drawer.uploadWorldMatrix(localRotation * machine().state.m_pose.getMatrix()).draw();

#if defined(_DEBUG)
        if (s_stopInput)
        {
            machine().props.input = {};
        }
        else
#endif
        {
            machine().props.input = UpdateCharacterAiLogic(m_logicState, machine());
        }

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

        ImmediateDrawer::Global().draw();

        debugUI();
    }

    void resetPhysicsState()
    {
        machine().state = {};

        machine().state.m_pose.position = GetRaceContext().stageManager().startPosition();

        machine().state.m_forwardVector = GetRaceContext().stageManager().courseSegments()[0].midwayStrips[0].toNext;

        machine().state.m_durability = machine().props.maxDurability;
    }

    void resetPhysicsProps()
    {
        machine().props.machineId = 1; // TODO

        machine().props.peakVelocity = 100.0f;

        machine().props.accelFactor = 1.0f;
    }

    void debugUI()
    {
        ImGui::Begin("Character AI");

        ImGui::Checkbox("Stop Input", &s_stopInput);

        ImGui::Checkbox("Stop Move", &s_stopMove);

        ImGui::SameLine();

        if (ImGui::Button("Step"))
        {
            UpdateMachinePhysicsState(machine().state, machine().props);
        }

        if (ImGui::DragFloat3("Position", &machine().state.m_pose.position.x))
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
            // machine().state.m_velocity = {};
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
            // machine().state.m_velocity = {};
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

    std::shared_ptr<GameObjectBase> CharacterAi::asGameObject() const
    {
        return p_impl;
    }
}
