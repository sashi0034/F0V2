#include "pch.h"
#include "RaceScene.h"

#include "IRaceContext.h"
#include "RaceContextState.h"
#include "Common/RaceSharedState.h"
#include "Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "TY/Scene.h"
#include "TY_Extension/CoroutineActor.h"

using namespace Race;

namespace
{
    IRaceContext* s_raceContext = nullptr;
}

struct RaceScene::Impl : ActorBase, IRaceContext
{
    ActorContainer m_children{};

    CoroutineActor m_coro{};

    RaceContextState m_state{};

    StageManager m_stageManager{};

    Impl(bool context)
    {
        if (context)
        {
            s_raceContext = this;
        }
    }

    ~Impl()
    {
        if (s_raceContext == this)
        {
            s_raceContext = nullptr;
        }
    }

    void Init()
    {
        m_stageManager = m_children.birth(StageManager());
        m_stageManager.init();
    }

    void update() override
    {
        m_children.updateEach();

        Graphics3D::SetViewMatrix(m_state.camera.viewMatrix());

        {
            auto projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                g_sharedState->fovFarZ
            );

            Graphics3D::SetProjectionMatrix(projectionMat);
        }

        m_state.cb.lambert->lightDirection = m_state.camera.worldMatrix().forward();
        m_state.cb.lambert->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_state.cb.lambert.upload();

        ImGui::Begin("Race Scene");

        ImGui::Text("This is a combat scene.");

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    RaceContextState& state() override
    {
        return m_state;
    }

    const RaceContextState& state() const override
    {
        return m_state;
    }
};

namespace Race
{
    RaceScene::RaceScene(bool context) :
        p_impl(std::make_shared<Impl>(context))
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

    IRaceContext& GetRaceContext()
    {
        assert(s_raceContext != nullptr);
        return *s_raceContext;
    }

    RaceContextState& GetRaceContextState()
    {
        assert(s_raceContext != nullptr);
        return s_raceContext->state();
    }
}
