#include "pch.h"
#include "RaceScene.h"

#include "IRaceContext.h"
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

    SimpleCamera3D m_camera{};

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

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        {
            auto projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                g_sharedState->fovFarZ
            );

            Graphics3D::SetProjectionMatrix(projectionMat);
        }

        ImGui::Begin("Race Scene");

        ImGui::Text("This is a combat scene.");

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    SimpleCamera3D& camera() override
    {
        return m_camera;
    }

    const SimpleCamera3D& camera() const override
    {
        return m_camera;
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
}
