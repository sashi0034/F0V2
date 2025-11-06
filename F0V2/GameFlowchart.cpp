#include "pch.h"
#include "GameFlowchart.h"

#include "Asset.generated.h"
#include "GamePalette.h"
#include "Editor/EditorScene.h"
#include "GM/DebugService.h"
#include "Race/RaceScene.h"
#include "TY/ActorContainer.h"
#include "TY/GpuMetrics.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/System.h"
#include "TY_Extension/AwaiterContext.h"
#include "TY_Extension/GameObjectBase.h"

namespace
{
    struct IFlowchart
    {
        virtual ~IFlowchart() = default;

        virtual std::unique_ptr<IFlowchart> Process(AwaiterContext& await, ActorContainer& parent) = 0;
    };

    std::unique_ptr<IFlowchart> CreateRaceFlowchart();

    struct EditorFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> EditorFlowchart::Process(AwaiterContext& await, ActorContainer& parent) override
        {
            auto editor = parent.birth(Editor::EditorScene());
            editor.init();

            await.waitForTrue([this, &editor]()
            {
#if defined(_DEBUG)
                if (not g_debugService.editorEnabled)
                {
                    return true;
                }
#endif

                return not editor.isAlive();
            });

            editor.kill();

            return CreateRaceFlowchart();
        }
    };

    struct RaceFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> Process(AwaiterContext& await, ActorContainer& parent) override
        {
            auto race = parent.birth(Race::RaceScene(true));
            race.init();

            await.waitForTrue([this, &race]()
            {
#if defined(_DEBUG)
                if (g_debugService.editorEnabled)
                {
                    return true;
                }
#endif

                return not race.isAlive();
            });

            race.kill();

            return std::make_unique<EditorFlowchart>();
        }
    };

    std::unique_ptr<IFlowchart> CreateRaceFlowchart()
    {
        return std::make_unique<RaceFlowchart>();
    }
}

struct F0V2::GameFlowchart::Impl : GameObjectBase
{
    ActorContainer m_children{};

    CoroutineActor m_flowchartCoroutine{};

    void Init()
    {
        restartFlowchart();
    }

private:
    void update() override
    {
        if (not m_flowchartCoroutine.isAlive())
        {
            restartFlowchart();
        }

        debugUI();

        m_children.updateEach();
    }

    void restartFlowchart()
    {
        m_flowchartCoroutine.kill();
        m_flowchartCoroutine = StartCoroutine(m_children, [this](AwaiterContext& await)
        {
            handleFlowchart(await);
        });
    }

    void handleFlowchart(AwaiterContext& await)
    {
        std::unique_ptr<IFlowchart> flowchart = std::make_unique<EditorFlowchart>();
#if 0
        const auto entryPoint = GetTomlDebugValueOf<String>(U"entry_point").lowercase();
        if (entryPoint == U"Quest"_s.lowercase()) flowchart = std::make_unique<QuestFlowchart>();
        else if (entryPoint == U"Exposition"_s.lowercase()) flowchart = std::make_unique<ExpositionFlowchart>();
#endif

        while (true)
        {
            if (flowchart == nullptr)
            {
                break;
            }

            flowchart = flowchart->Process(await, m_children);

            await.waitForFrames(1);
        }
    }

    void debugUI()
    {
        ImGui::Begin("System Window");

        static bool s_sleep{};;
        if (ImGui::Checkbox("Sleep", &s_sleep); s_sleep)
        {
            System::Sleep(500);
        }

        ImGui::PushStyleColor(ImGuiCol_Button, GamePalette::DarkOrange.toFloat4().cast<ImVec4>());
        {
            if (ImGui::Button("Toggle Editor"))
            {
                g_debugService.editorEnabled = not g_debugService.editorEnabled;
            }
        }
        ImGui::PopStyleColor();

        ImGui::Text("GPU Memory Usage: %.2f MB", GpuMetrics::MemoryUsage().estimateLocalUsageInMB());

        ImGui::Text("Mouse Position: (%.2f, %.2f)", Mouse::PosF().x, Mouse::PosF().y);

        ImGui::SeparatorText("g_debugService");

        ImGui::Checkbox("editorEnabled", &g_debugService.editorEnabled);

        ImGui::SliderFloat("cameraSpeed", &g_debugService.cameraSpeed, 1.0f, 10.0f);

        ImGui::InputInt("monitorMachineId", &g_debugService.monitorMachineId);

        ImGui::Checkbox("diablePlayerInput", &g_debugService.disablePlayerInput);

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"GameFlowchart";
    }
};

namespace F0V2
{
    GameFlowchart::GameFlowchart() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void GameFlowchart::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> GameFlowchart::asGameObject() const
    {
        return p_impl;
    }
}
