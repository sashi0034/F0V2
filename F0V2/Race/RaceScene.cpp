#include "pch.h"
#include "RaceScene.h"

#include "IRaceContext.h"
#include "RaceCameraController.h"
#include "RaceContextContent.h"
#include "Ai/CharacterAi.h"
#include "Common/RaceSharedState.h"
#include "Player/Player.h"
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

    RaceContextContent m_state{};

    StageManager m_stageManager{};

    RaceCameraController m_cameraController{};

    Player m_player{};

    Array<CharacterAi> m_characterAiList{};

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

        m_player = m_children.birth(Player());
        m_player.init();

        m_characterAiList.push_back(m_children.birth(CharacterAi()));
        m_characterAiList.back().init();
    }

    void update() override
    {
        m_cameraController.update();

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

        m_children.updateEach();

        m_state.cb.lambert->lightDirection = m_state.camera.worldMatrix().forward();
        m_state.cb.lambert->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_state.cb.lambert.upload();

        ImGui::Begin("Race Scene");

        ImGui::Text("This is a race scene.");

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    RaceContextContent& state() override
    {
        return m_state;
    }

    const RaceContextContent& state() const override
    {
        return m_state;
    }

    StageManager& stageManager() override
    {
        return m_stageManager;
    }

    const StageManager& stageManager() const override
    {
        return m_stageManager;
    }

    const MachineUnit& getMachine(int id) const override
    {
        return m_player.machine(); // TODO: Ai
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

    RaceContextContent& GetRaceContextContent()
    {
        assert(s_raceContext != nullptr);
        return s_raceContext->state();
    }
}
