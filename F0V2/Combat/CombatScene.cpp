#include "pch.h"
#include "CombatScene.h"

#include "TY/ActorContainer.h"
#include "TY/Logger.h"
#include "TY_Extension/AwaiterContext.h"
#include "TY_Extension/CoroutineActor.h"

using namespace Combat;

struct CombatScene::Impl : ActorBase
{
    ActorContainer m_children{};

    CoroutineActor m_coro{};

    void Init()
    {
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

        ImGui::Begin("Combat Scene");

        ImGui::Text("This is a combat scene.");

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Combat
{
    CombatScene::CombatScene() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CombatScene::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> CombatScene::asActor() const
    {
        return p_impl;
    }
}
