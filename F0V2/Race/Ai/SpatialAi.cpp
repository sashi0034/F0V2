#include "pch.h"
#include "SpatialAi.h"

#include "SpatialAiLogic.h"
#include "TY/ActorContainer.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
}

struct SpatialAi::Impl : GameObjectBase
{
    ActorContainer m_children{};

    SpatialAiLogicState m_logicState{};

    void Init()
    {
        m_logicState = BuildSpatialAiLogic();
    }

private:
    void update() override
    {
        debugUI();
    }

    void debugUI()
    {
        ImGui::Begin("Spatial AI");

        ImGui::Text(std::format("Waypoints: {}", m_logicState.nodes.size()).c_str());

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"SpatialAi";
    }
};

namespace Race
{
    SpatialAi::SpatialAi() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void SpatialAi::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    const Array<SpatialWaypoint>& SpatialAi::waypoints() const
    {
        return p_impl->m_logicState.nodes;
    }

    std::shared_ptr<GameObjectBase> SpatialAi::asGameObject() const
    {
        return p_impl;
    }
}
