#include "pch.h"
#include "CharacterAI.h"

#include "Asset.generated.h"
#include "CharacterAILogic.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/RaceSharedState.h"
#include "Race/Machine/MachineDrawer.h"
#include "Race/Machine/MachinePhysics.h"
#include "TY/ActorContainer.h"
#include "TY/ImmediateDrawer.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Random.h"
#include "Util/DebugTomlValue.h"

using namespace Race;

namespace
{
#if defined(_DEBUG)
    bool s_stopInput{};
    bool s_stopMove{};
#endif
}

struct CharacterAI::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::string m_debugName = "CharacterAI";
#endif

    int m_aiId{};

    ActorContainer m_children{};

    MachineDrawer m_drawer{};

    CharacterAILogicState m_logicState{};

    void Init(int aiId)
    {
        GetRaceContext().registerDrawer(shared_from_this());

        m_aiId = aiId;
        m_logicState.m_aiId = aiId;

#if defined(_DEBUG)
        m_debugName += "#" + std::to_string(aiId);
#endif

        auto color = Palette::List()[Random::Int(0, Palette::List().size() - 1)];
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("fix_ai_color"))
        {
            color = Palette::SandyBrown;
        }
#endif
        machine().props.themeColor = color;
        m_drawer.init(MachineId(), color.sRGBToLinear());

        resetPhysicsProps();
        resetPhysicsState();

#if defined(_DEBUG) && 0
        g_debugService.monitorMachineId = 1;
#endif
    }

    int MachineId() const
    {
        return 1 + m_aiId;
    }

private:
    MachinePhysicsUnit& machine() const
    {
        return GetRaceContext().machineManager().fetchMachine(MachineId());
    }

    void update() override
    {
#if defined(_DEBUG)
        if (s_stopInput)
        {
            machine().props.input = {};
        }
        else
#endif
        {
            machine().props.input = UpdateCharacterAILogic(m_logicState, machine());
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
            GetRaceContext().machineManager().eventHandler().handleIfNeeded(machine().id());
        }

        m_drawer.update();

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
        machine().props.machineId = MachineId();

        machine().props.peakVelocity = 200.0f;

        machine().props.accelFactor = 0.5f;
    }

    void debugUI()
    {
#if defined(_DEBUG)
        if (m_aiId != 0)
        {
            return;
        }

        ImGui::Begin(std::format("Character AI [{}]", m_aiId).c_str());

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
#endif
    }

    void killed() override
    {
        m_children.killEach();

        GetRaceContext().unregisterDrawer(this);
    }
};

namespace Race
{
    CharacterAI::CharacterAI() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CharacterAI::init(int aiId)
    {
        p_impl->Init(aiId);
    }

    MachineId CharacterAI::machineId() const
    {
        return p_impl->MachineId();
    }

    void CharacterAI::setInputCommand(const CharacterAIInputCommand& command)
    {
        p_impl->m_logicState.m_inputCommand = command;
    }

    std::shared_ptr<ActorBase> CharacterAI::asActor() const
    {
        return p_impl;
    }
}
