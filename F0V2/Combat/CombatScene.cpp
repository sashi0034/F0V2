#include "pch.h"
#include "CombatScene.h"

#include "TY/ActorContainer.h"

using namespace Combat;

struct CombatScene::Impl : ActorBase
{
    ActorContainer m_children{};

    void update() override
    {
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

    std::shared_ptr<ActorBase> CombatScene::asActor() const
    {
        return p_impl;
    }
}
