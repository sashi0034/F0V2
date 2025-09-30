#include "pch.h"
#include "RaceScene.h"

#include "Debug/DebugEditor.h"
#include "Debug/DebugPlayground.h"
#include "Toys/ToyCinnamon.h"
#include "TY/ActorContainer.h"
#include "TY/Logger.h"
#include "TY_Extension/AwaiterContext.h"
#include "TY_Extension/CoroutineActor.h"

using namespace Race;

struct RaceScene::Impl : ActorBase
{
    ActorContainer m_children{};

    DebugEditor m_debugEditor{};

    CoroutineActor m_coro{};

    void Init()
    {
        m_debugEditor = m_children.birth(DebugEditor());
        m_debugEditor.init();

        // auto cinnamon = m_children.birth(ToyCinnamon());
        // cinnamon.init();

        m_coro = StartCoroutine(m_children, [this](AwaiterContext& await)
        {
            await.waitForTime(2.0s);

            LogInfo("Hello!");

            await.waitForTime(2.0s);

            LogInfo("World!");
        });
    }

    void update() override
    {
        m_children.updateEach();

        ImGui::Begin("Race Scene");

        ImGui::Text("This is a combat scene.");

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    RaceScene::RaceScene() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceScene::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> RaceScene::asActor() const
    {
        return p_impl;
    }
}
