#include "pch.h"
#include "RaceController.h"

#include "Asset.generated.h"
#include "IRaceContext.h"
#include "RaceControlState.h"
#include "Common/CourseFileInfo.h"
#include "Common/RaceSharedState.h"
#include "UI/RaceUIController.h"
#include "TY/ActorContainer.h"
#include "TY/Gamepad.h"
#include "TY/KeyboardInput.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "TY_Extension/TaskUtils.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    class ReverseDirectionChecker
    {
    public:
        void Update(const MachinePhysicsState& state)
        {
            if (state.m_lapProgress != m_previousLapProgress)
            {
                if (state.m_lapProgress.isLessThan(m_previousLapProgress))
                {
                    m_reverseCount++;
                }
                else
                {
                    m_reverseCount = 0;
                }
            }

            m_previousLapProgress = state.m_lapProgress;
        }

        bool IsReversing() const
        {
            return m_reverseCount >= 15;
        }

    private:
        int m_reverseCount{};
        LapProgress m_previousLapProgress{};
    };
}

struct RaceController::Impl : ActorBase
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceController"};
#endif

    ActorContainer m_children{};

    RaceUIController m_uiController{};

    CoroutineActor m_raceFlowCoroutine{};

    RaceControlState m_raceControlState{};

    CoroutineActor m_centerBannerCoroutine{};

    bool m_playerCrashed{};

    MusicAudio m_backgroundMusic{};

    ReverseDirectionChecker m_reverseDirectionChecker{};

    void Init()
    {
        g_sharedState->isRaceEnded = false;

        m_raceFlowCoroutine = StartCoroutine(m_children, [this](AwaitContext& await)
        {
            processRaceFlow(await);
        });

        m_uiController = m_children.birth(RaceUIController());
        m_uiController.init(m_raceControlState);
    }

private:
    void update() override
    {
        m_children.updateEach();

#if defined(_DEBUG) && 1
        ImmediatePrint("Music: {:.03f}s", Asset_music::MetropolitanBreeze_loop().posSec());
#endif

        // ゲームオーバーチェック
        const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
        if (not m_raceControlState.m_raceFinished && player.state.isDead() && not m_playerCrashed)
        {
            m_raceFlowCoroutine.kill();

            m_playerCrashed = true;

            popupCenterBanner(RaceCenterBanner::YourMachineHasCrashed, U"Your Machine Has Crashed !", -1);

            runCompleteProcess(3.0f);
        }

        if (g_sharedState->isRaceStarted && not m_raceControlState.m_raceFinished)
        {
            int currentLap = player.state.m_markedLapProgress.lapIndex;
            if (InRange<int>(currentLap, 0, m_raceControlState.m_measuredLapTimes.size() - 1))
            {
                m_raceControlState.m_measuredLapTimes[currentLap] += InGameDeltaTime();
            }
        }

        debugUI();

        // 逆走チェック
        m_reverseDirectionChecker.Update(player.state);
        m_raceControlState.m_isReversing = m_reverseDirectionChecker.IsReversing();

        // チュートリアル更新
        if (m_raceControlState.m_boostTutorialEnabled)
        {
            bool done;
            if (IsUsingGamepad())
            {
                done = MainGamepad.b().down;
            }
            else
            {
                done = KeySpace.down();
            }

            if (done)
            {
                m_raceControlState.m_boostTutorialEnabled = false;
            }
        }

        // TODO: リタイア本実装
        {
            // 仮実装: ESC 長押しでリタイア
            static float s_duration{};
            s_duration = KeyEscape.pressed() ? s_duration + InGameDeltaTime() : 0.0f;
            if (s_duration > 0.5f)
            {
                s_duration = 0.0f;
                g_sharedState->isRaceEnded = true;
            }
        }
    }

    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("Race Flow Controller");

        if (ImGui::Button("End"))
        {
            g_sharedState->isRaceEnded = true;
        }

        ImGui::End();
#endif
    }

    void processRaceFlow(AwaitContext& await)
    {
        g_sharedState->isRaceStarted = false;

        await.waitForFrames(1);

        m_backgroundMusic = GetRaceContext().courseFileInfo().music;
        m_backgroundMusic.play(); // TODO
        m_backgroundMusic.setLoop(GetRaceContext().courseFileInfo().musicLoopRanges[0]);

        process321Go(await);

        g_sharedState->isRaceStarted = true;

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_markedLapProgress.lapIndex == 1;
        });

        popupCenterBanner(RaceCenterBanner::YouGotBoostPower, U"You've Got Boost Power !", 5.0f);

        m_raceControlState.m_boostTutorialEnabled = true;

        Asset_sound::GotBoostPower().playOneShot();

        m_backgroundMusic.setLoop(GetRaceContext().courseFileInfo().musicLoopRanges[1]);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_markedLapProgress.lapIndex == 2;
        });

        popupCenterBanner(RaceCenterBanner::TheFinalLap, U"The Final Lap !", 5.0f);

        Asset_sound::FinalRap().playOneShot();

        m_backgroundMusic.setLoop(GetRaceContext().courseFileInfo().musicLoopRanges[2]);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_markedLapProgress.lapIndex == 3; // m_lapProgress のほうが面白いかも?
        });

#if defined(_DEBUG)
        await.waitForTrue([]
        {
            return not GetDebugTomlValue<bool>("disable_finish");
        });
#endif

        popupCenterBanner(RaceCenterBanner::Finish, U"Finish !", -1);

        m_raceControlState.m_raceFinished = true;

        const int playerRank = GetRaceContext().machineManager().getEvaluation(PlayerMachineId).rank;

        Asset_sound::Finish().playOneShot();
        Asset_music::Shiro().play();

        await.waitForTime(2.5s);

        m_raceControlState.m_playerFinalRank = playerRank;

        runCompleteProcess(5.0f);
    }

    void process321Go(AwaitContext& await)
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("skip_321go"))
        {
            g_sharedState->isRaceStarted = true;
            return;
        }
#endif

        m_raceControlState.m_startCountdown = 3;
        Asset_sound::CountThree().playOneShot();

        await.waitForTime(1.0f);

        m_raceControlState.m_startCountdown--;
        Asset_sound::CountTwo().playOneShot();

        await.waitForTime(1.0f);

        m_raceControlState.m_startCountdown--;
        Asset_sound::CountOne().playOneShot();

        await.waitForTime(1.0f);

        m_raceControlState.m_startCountdown--;
        Asset_sound::CountGo().playOneShot();

        popupCenterBanner(RaceCenterBanner::Go, U"Go !", 3.0f);
    }

    void popupCenterBanner(RaceCenterBanner banner, const std::u32string& message, float seconds)
    {
        m_raceControlState.m_centerBanner = banner;

        m_centerBannerCoroutine.kill();
        m_centerBannerCoroutine = StartCoroutine(m_children, [this, banner, message, seconds](AwaitContext& await)
        {
            const auto messages = SplitStringView(message, U' ');
            m_raceControlState.m_centerBannerMessage = {};
            constexpr float wordDelay = 0.1f;
            for (const auto& next : messages)
            {
                if (not m_raceControlState.m_centerBannerMessage.empty())
                {
                    m_raceControlState.m_centerBannerMessage += U' ';
                }

                m_raceControlState.m_centerBannerMessage += {next.data(), next.size()};

                await.waitForTime(wordDelay);
            }

            if (seconds < 0.0f)
            {
                return;
            }

            await.waitForTime(seconds - (messages.size() * wordDelay));

            if (m_raceControlState.m_centerBanner == banner)
            {
                m_raceControlState.m_centerBanner = RaceCenterBanner::None;
            }
        });
    }

    void runCompleteProcess(float waitSeconds)
    {
        StartCoroutine(m_children, [this, waitSeconds](AwaitContext& await)
        {
            await.waitForTime(waitSeconds);

            (void)await.waitAnyTrue({
                    // [0]
                    []()
                    {
                        return IsUsingGamepad() ? MainGamepad.a().down : KeySpace.down();
                    },
                    // [1]
                    MakeTimeoutTask(60.0s)
                }
            );

            Audio::StopMusic();

            g_sharedState->isRaceEnded = true;
        });
    }

    void killed() override
    {
        m_children.killEach();

        Audio::StopAllSounds();
        Audio::StopMusic();
    }
};

namespace Race
{
    RaceController::RaceController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceController::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> RaceController::asActor() const
    {
        return p_impl;
    }
}
