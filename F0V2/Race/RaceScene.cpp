#include "pch.h"
#include "RaceScene.h"

#include "IRaceContext.h"
#include "RaceCameraController.h"
#include "RaceContextContent.h"
#include "RaceDrawManager.h"
#include "RaceController.h"
#include "AI/CharacterAI.h"
#include "AI/MetaAI.h"
#include "AI/SpatialAI.h"
#include "Common/CourseFileInfo.h"
#include "Common/RaceSharedState.h"
#include "Vfx/MachineVfxEmitter.h"
#include "Vfx/RaceVfxDrawer.h"
#include "Player/Player.h"
#include "Stage/StageManager.h"
#include "TY/ActorContainer.h"
#include "TY/Graphics3D.h"
#include "Util/DebugTomlValue.h"

using namespace Race;

namespace
{
    IRaceContext* s_raceContext = nullptr;
}

struct RaceScene::Impl : ActorBase, IRaceContext
{
    ActorContainer m_children{};

    RaceContextContent m_state{};

    CourseFileInfo m_courseFileInfo{};

    RaceDrawManager m_drawManager{};

    RaceVfxDrawer m_vfxDrawer{};

    RaceController m_raceController{};

    StageManager m_stageManager{};

    MachineManager m_machineManager{};

    RaceCameraController m_cameraController{};

    Player m_player{};

    SpatialAI m_spatialAI{};

    Array<CharacterAI> m_characterAIList{};

    MetaAI m_metaAI{};

    MachineVfxEmitter m_machineVfxEmitter{};

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
        m_courseFileInfo = GetCourseFileInfoByPath(g_sharedState->coursePath);

        m_cameraController = m_children.birth(RaceCameraController());

        m_drawManager = m_children.birth(RaceDrawManager());
        m_drawManager.init();

        m_vfxDrawer = m_children.birth(RaceVfxDrawer());
        m_vfxDrawer.init();

        m_raceController = m_children.birth(RaceController());
        m_raceController.init();

        m_stageManager = m_children.birth(StageManager());
        m_stageManager.init();

        m_machineManager = m_children.birth(MachineManager());
        m_machineManager.init();

        m_player = m_children.birth(Player());
        m_player.init();

        m_spatialAI = m_children.birth(SpatialAI());
        m_spatialAI.init();

        int aiCount = 98;
#if defined(_DEBUG)
        if (int aiCount_ = GetDebugTomlValue<int>("ai_count", -1); aiCount >= 0)
        {
            aiCount = aiCount_;
        }
#endif

        for (int i = 0; i < aiCount; ++i)
        {
            m_characterAIList.push_back(m_children.birth(CharacterAI()));
            m_characterAIList.back().init(i);
        }

        m_metaAI = m_children.birth(MetaAI());
        m_metaAI.init();

        m_machineVfxEmitter = m_children.birth(MachineVfxEmitter());
        m_machineVfxEmitter.init();
    }

    void update() override
    {
        m_children.updateEach();

        m_state.cb.lambert->lightDirection = m_state.camera.worldMatrix().forward();
        m_state.cb.lambert->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_state.cb.lambert.upload();
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

    const CourseFileInfo& courseFileInfo() const override
    {
        return m_courseFileInfo;
    }

    void registerDrawer(const std::shared_ptr<IRaceDrawer>& drawer) override
    {
        m_drawManager.registerDrawer(drawer);
    }

    void unregisterDrawer(const IRaceDrawer* drawer) override
    {
        m_drawManager.unregisterDrawer(drawer);
    }

    RaceVfxDrawer& vfxDrawer() override
    {
        return m_vfxDrawer;
    }

    const RaceVfxDrawer& vfxDrawer() const override
    {
        return m_vfxDrawer;
    }

    StageManager& stageManager() override
    {
        return m_stageManager;
    }

    const StageManager& stageManager() const override
    {
        return m_stageManager;
    }

    MachineManager& machineManager() override
    {
        return m_machineManager;
    }

    const MachineManager& machineManager() const override
    {
        return m_machineManager;
    }

    SpatialAI& spatialAI() override
    {
        return m_spatialAI;
    }

    const SpatialAI& spatialAI() const override
    {
        return m_spatialAI;
    }

    Array<CharacterAI>& characterAIList() override
    {
        return m_characterAIList;
    }

    const Array<CharacterAI>& characterAIList() const override
    {
        return m_characterAIList;
    }

    // MetaAI& metaAI() override
    // {
    //     return m_metaAI;
    // }
    //
    // const MetaAI& metaAI() const override
    // {
    //     return m_metaAI;
    // }
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
