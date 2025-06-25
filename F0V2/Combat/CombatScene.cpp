#include "pch.h"
#include "CombatScene.h"

#include "Util/ActorContainer.h"

using namespace Combat;

struct CombatScene::Impl : ActorBase
{
    ActorContainer m_children{};

    void Update() override
    {
        ImGui::Begin("Combat Scene");

        ImGui::Text("This is a combat scene.");

        ImGui::End();
    }

    void Killed() override
    {
        m_children.KillEach();
    }
};

namespace Combat
{
    CombatScene::CombatScene() :
        p_impl(std::make_shared<Impl>())
    {
    }

    std::shared_ptr<ActorBase> CombatScene::AsActor() const
    {
        return p_impl;
    }
}
