#include "pch.h"
#include "CharacterAi.h"

#include "Asset.generated.h"
#include "CharacterAiLogic.h"
#include "GM/DebugService.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
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

struct CharacterAi::Impl : ActorBase
{
#if defined(_DEBUG)
    std::string m_debugName = "CharacterAi";
#endif

    int m_aiId{};

    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    CharacterAiLogicState m_logicState{};

    void Init(int aiId)
    {
        m_aiId = aiId;
        m_logicState.m_aiId = aiId;
        m_debugName += "#" + std::to_string(aiId);

        auto color = Palette::List()[Random::Int(0, Palette::List().size() - 1)];
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("fix_ai_color"))
        {
            color = Palette::SandyBrown;
        }
#endif

        static const auto s_modelData =
            PrimitiveModel3D::Capsule(machine().state.m_radius, machine().state.m_height, color);

        auto modelData = s_modelData;
        modelData.materials[0].parameters.diffuse = color.toFloat3();

        const ModelBuffer model = ModelBuffer{modelData};

        m_drawer =
            ModelDrawerParams{}
            .setModel(model)
            .setShader(Asset_shader::lambert)
            .setCbv10AndLater({GetRaceContextContent().cb.lambert});

        resetPhysicsProps();
        resetPhysicsState();

#if defined(_DEBUG) && 0
        g_debugService.monitorMachineId = 1;
#endif
    }

private:
    MachinePhysicsUnit& machine() const
    {
        return GetRaceContext().machineManager().fetchMachine(machineId());
    }

    int machineId() const
    {
        return 1 + m_aiId;
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
        SetupMachinePhysicsState(machine().state, machine().props);
    }

    void resetPhysicsProps()
    {
        machine().props.machineId = machineId();

        machine().props.peakVelocity = 100.0f;

        machine().props.accelFactor = 1.0f;
    }

    void debugUI()
    {
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
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    CharacterAi::CharacterAi() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CharacterAi::init(int aiId)
    {
        p_impl->Init(aiId);
    }

    std::shared_ptr<ActorBase> CharacterAi::asActor() const
    {
        return p_impl;
    }
}
