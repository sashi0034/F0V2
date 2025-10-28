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

    SpatialData m_data{};

    void Init()
    {
        m_data = BuildSpatialData();
    }

private:
    void update() override
    {
        debugUI();
    }

    void debugUI()
    {
        ImGui::Begin("Spatial AI");

        ImGui::Text(std::format("Waypoints: {}", m_data.waypoints.size()).c_str());

        if (ImGui::Button("Rebuild m_data"))
        {
            m_data = BuildSpatialData();
        }

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

    const SpatialData& SpatialAi::data() const
    {
        return p_impl->m_data;
    }

    std::shared_ptr<GameObjectBase> SpatialAi::asGameObject() const
    {
        return p_impl;
    }
}
