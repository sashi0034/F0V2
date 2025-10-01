#include "pch.h"
#include "GameFlowchart.h"

#include "Asset.generated.h"
#include "Editor/EditorScene.h"
#include "Race/RaceScene.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
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

            await.waitForTrue([this, &editor]() { return not editor.isAlive(); });

            return CreateRaceFlowchart();
        }
    };

    struct RaceFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> Process(AwaiterContext& await, ActorContainer& parent) override
        {
            auto race = parent.birth(Race::RaceScene());

            await.waitForTrue([this, &race]() { return not race.isAlive(); });

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
