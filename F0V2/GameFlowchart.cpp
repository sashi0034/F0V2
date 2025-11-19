#include "pch.h"
#include "GameFlowchart.h"

#include "GameGlobalUI.h"
#include "GamePalette.h"
#include "Editor/EditorScene.h"
#include "GM/DebugService.h"
#include "GM/GamepadConfigModal.h"
#include "Race/RaceScene.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/CourseSegmentBuilder.h"
#include "Race/Common/RaceSharedState.h"
#include "RaceSetup/RaceSetupScene.h"
#include "TY/ActorContainer.h"
#include "TY/Audio.h"
#include "TY/Gamepad.h"
#include "TY/GpuMetrics.h"
#include "TY/KeyboardInput.h"
#include "TY/Mouse.h"
#include "TY/System.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "TY_Extension/GameObjectBase.h"
#include "Util/DebugTomlValue.h"

namespace
{
    std::string gamepadConfigPath = "save/gamepad.toml";

    struct IFlowchart
    {
        virtual ~IFlowchart() = default;

        virtual std::unique_ptr<IFlowchart> Process(AwaitContext& await, ActorContainer& parent) = 0;
    };

    void loadCourseData(const std::string& courseFilepath)
    {
        const auto course = Race::LoadCourseData(courseFilepath);

        Array<Race::CourseSegment> segments{};
        BuildCourseSegmentIfNeeded(segments, course.nodes);

        Race::g_sharedState->coursePath = courseFilepath;
        Race::g_sharedState->courseSegments = std::move(segments);
    }
}

struct Flowcharts
{
    struct EditorFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> EditorFlowchart::Process(AwaitContext& await, ActorContainer& parent) override
        {
            g_debugService.editorEnabled = true;

            auto editor = parent.birth(Editor::EditorScene());
            editor.init();

            await.waitForTrue([this, &editor]()
            {
#if defined(_DEBUG)
                if (not g_debugService.editorEnabled)
                {
                    return true;
                }
#endif

                return not editor.isAlive();
            });

            editor.kill();

            return std::make_unique<RaceFlowchart>();
        }
    };

    // -----------------------------------------------

    struct RaceSetupFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> Process(AwaitContext& await, ActorContainer& parent) override
        {
            auto raceSetup = parent.birth(RaceSetup::RaceSetupScene());
            raceSetup.init();

            await.waitForTrue([this, &raceSetup]()
            {
                return raceSetup.isConfirmed();
            });

            loadCourseData(raceSetup.selectedCourseFilepath());

            raceSetup.kill();

            return std::make_unique<RaceFlowchart>();
        }
    };

    // -----------------------------------------------

    struct RaceFlowchart : IFlowchart
    {
        std::unique_ptr<IFlowchart> Process(AwaitContext& await, ActorContainer& parent) override
        {
            auto race = parent.birth(Race::RaceScene(true));
            race.init();

            await.waitForTrue([this, &race]()
            {
#if defined(_DEBUG)
                if (g_debugService.editorEnabled)
                {
                    return true;
                }
#endif

                return Race::g_sharedState->isRaceEnded;
            });

            race.kill();

#if defined(_DEBUG)
            if (g_debugService.editorEnabled)
            {
                return std::make_unique<EditorFlowchart>();
            }
#endif

            return std::make_unique<RaceSetupFlowchart>();
        }
    };
};

struct F0V2::GameFlowchart::Impl : GameObjectBase
{
    ActorContainer m_children{};

    CoroutineActor m_flowchartCoroutine{};

    void Init()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile(gamepadConfigPath));

        restartFlowchart();
    }

private:
    void update() override
    {
#if defined(_DEBUG)
        Audio::SetEnabled(GetDebugTomlValue<bool>("enable_audio"));
#endif

        if (KeyF2.down())
        {
            GM::LaunchGamepadConfigModal(gamepadConfigPath);
        }

        if (not m_flowchartCoroutine.isAlive())
        {
            restartFlowchart();
        }

        DrawGameGlobalUI();

        m_children.updateEach();
    }

    void restartFlowchart()
    {
        m_flowchartCoroutine.kill();
        m_flowchartCoroutine = StartCoroutine(m_children, [this](AwaitContext& await)
        {
            handleFlowchart(await);
        });
    }

    void handleFlowchart(AwaitContext& await)
    {
        std::unique_ptr<IFlowchart> flowchart{};
#if defined(_DEBUG)
        if (const auto entryPoint = ToLowercase(GetDebugTomlValue<std::string>("entry_point"));
            not entryPoint.empty())
        {
            if (entryPoint == ToLowercase("Editor"))
            {
                flowchart = std::make_unique<Flowcharts::EditorFlowchart>();
            }
            else if (entryPoint == ToLowercase("RaceSetup"))
            {
                flowchart = std::make_unique<Flowcharts::RaceSetupFlowchart>();
            }
            else if (entryPoint == ToLowercase("Race"))
            {
                loadCourseData(GetDebugTomlValue<std::string>("fixed_course_path"));
                flowchart = std::make_unique<Flowcharts::RaceFlowchart>();
            }
        }
#endif

        if (not flowchart)
        {
            flowchart = std::make_unique<Flowcharts::RaceSetupFlowchart>();;
        }

        while (true)
        {
            if (flowchart == nullptr)
            {
                break;
            }

            flowchart = flowchart->Process(await, m_children);

            await.waitForFrames(1);
        }
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"GameFlowchart";
    }
};

inline namespace F0V2
{
    GameFlowchart::GameFlowchart() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void GameFlowchart::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> GameFlowchart::asGameObject() const
    {
        return p_impl;
    }
}
