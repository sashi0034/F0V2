#include "pch.h"
#include "RaceFlowController.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "GameGlobalUI.h"
#include "GamePalette.h"
#include "IRaceContext.h"
#include "IRaceDrawer.h"
#include "Common/CourseFileInfo.h"
#include "Common/RaceSharedState.h"
#include "UI/UI_DurabilityBar.h"
#include "UI/UI_LabelText.h"
#include "TY/ActorContainer.h"
#include "TY/Gamepad.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/KeyboardInput.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "TY_Extension/TaskUtils.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    enum class MajorBanner : uint8_t
    {
        None,
        Go,
        YouGotBoostPower,
        TheFinalLap,
        Finish,
        YourMachineHasCrashed,
    };

    std::string formatLapTime(float timeSec)
    {
        const int minutes = static_cast<int>(timeSec / 60);
        const int seconds = static_cast<int>(std::fmod(timeSec, 60));
        const int hundredths = static_cast<int>(std::fmod(timeSec, 1.0f) * 100.0f);

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << minutes << '\''
            << std::setw(2) << seconds << "''"
            << std::setw(2) << hundredths;
        return oss.str();
    }

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

struct RaceFlowController::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceFlowController"};
#endif

    ActorContainer m_children{};

    UI_DurabilityBar m_durabilityBar{};

    CoroutineActor m_raceFlowCoroutine{};

    int m_countdown{};

    MajorBanner m_majorBanner{MajorBanner::None};

    std::u32string m_majorBannerMessage{};

    CoroutineActor m_majorBannerCoroutine{};

    int m_playerFinalRank{-1};

    bool m_raceFinished{};
    bool m_playerCrashed{};

    std::array<float, 3> m_measuredLapTimes{};

    MusicAudio m_backgroundMusic{};

    ReverseDirectionChecker m_reverseDirectionChecker{};

    bool m_boostTutorialEnabled{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        g_sharedState->isRaceEnded = false;

        m_raceFlowCoroutine = StartCoroutine(m_children, [this](AwaitContext& await)
        {
            processRaceFlow(await);
        });

        m_durabilityBar = m_children.birth(UI_DurabilityBar());
        m_durabilityBar.init();
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
        if (not m_raceFinished && player.state.isDead() && not m_playerCrashed)
        {
            m_raceFlowCoroutine.kill();

            m_playerCrashed = true;

            popupMajorBanner(MajorBanner::YourMachineHasCrashed, U"Your Machine Has Crashed !", -1);

            runCompleteProcess(3.0f);
        }

        if (not m_raceFinished)
        {
            int currentLap = player.state.m_markedLapProgress.lapIndex;
            if (InRange<int>(currentLap, 0, m_measuredLapTimes.size() - 1))
            {
                m_measuredLapTimes[currentLap] += InGameDeltaTime();
            }
        }

        debugUI();

        // 逆走チェック
        m_reverseDirectionChecker.Update(player.state);

        // チュートリアル更新
        if (m_boostTutorialEnabled && (IsUsingGamepad() && MainGamepad.b().down))
        {
            m_boostTutorialEnabled = false;
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

        popupMajorBanner(MajorBanner::YouGotBoostPower, U"You've Got Boost Power !", 5.0f);

        m_boostTutorialEnabled = true;

        Asset_sound::GotBoostPower().playOneShot();

        m_backgroundMusic.setLoop(GetRaceContext().courseFileInfo().musicLoopRanges[1]);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_markedLapProgress.lapIndex == 2;
        });

        popupMajorBanner(MajorBanner::TheFinalLap, U"The Final Lap !", 5.0f);

        Asset_sound::FinalRap().playOneShot();

        m_backgroundMusic.setLoop(GetRaceContext().courseFileInfo().musicLoopRanges[2]);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 3;
        });

#if defined(_DEBUG)
        await.waitForTrue([]
        {
            return not GetDebugTomlValue<bool>("disable_finish");
        });
#endif

        popupMajorBanner(MajorBanner::Finish, U"Finish !", -1);

        m_raceFinished = true;

        const int playerRank = GetRaceContext().machineManager().getEvaluation(PlayerMachineId).rank;

        Asset_sound::Finish().playOneShot();
        Asset_music::Shiro().play();

        await.waitForTime(2.5s);

        m_playerFinalRank = playerRank;

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

        m_countdown = 3;
        Asset_sound::CountThree().playOneShot();

        await.waitForTime(1.0f);

        m_countdown--;
        Asset_sound::CountTwo().playOneShot();

        await.waitForTime(1.0f);

        m_countdown--;
        Asset_sound::CountOne().playOneShot();

        await.waitForTime(1.0f);

        m_countdown--;
        Asset_sound::CountGo().playOneShot();

        popupMajorBanner(MajorBanner::Go, U"Go !", 3.0f);
    }

    void popupMajorBanner(MajorBanner banner, const std::u32string& message, float seconds)
    {
        m_majorBanner = banner;

        m_majorBannerCoroutine.kill();
        m_majorBannerCoroutine = StartCoroutine(m_children, [this, banner, message, seconds](AwaitContext& await)
        {
            const auto messages = SplitStringView(message, U' ');
            m_majorBannerMessage = {};
            constexpr float wordDelay = 0.1f;
            for (const auto& next : messages)
            {
                if (not m_majorBannerMessage.empty())
                {
                    m_majorBannerMessage += U' ';
                }

                m_majorBannerMessage += {next.data(), next.size()};

                await.waitForTime(wordDelay);
            }

            if (seconds < 0.0f)
            {
                return;
            }

            await.waitForTime(seconds - (messages.size() * wordDelay));

            if (m_majorBanner == banner)
            {
                m_majorBanner = MajorBanner::None;
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

    void drawUI() const override
    {
        const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];

        // スピードメーター
        DrawLabelText(ToUtf32(std::format("{:.0f} km/h", player.state.m_velocity.length() * 10.0f)),
                      28.0f,
                      Screen::SizeF().movedBy(-32.0f, -20.0f),

                      Alignment9::BottomRight);

        // -----------------------------------------------
        // 耐久値バー
        {
            m_durabilityBar.draw();
        }

        // -----------------------------------------------
        // 順位

        if (not m_raceFinished)
        {
            const auto evaluation = GetRaceContext().machineManager().getEvaluation(PlayerMachineId);
            const int rank1 = evaluation.rank + 1;
            const int aliveMachines = GetRaceContext().machineManager().aliveMachineCount();
            DrawLabelText(ToUtf32(std::format("{}", rank1)),
                          64.0f,
                          Screen::TopCenterF().movedY(40.0f),
                          Alignment9::TopCenter);

            Immediate2D::Rect{
                    RectF{Screen::TopCenterF().movedY(110.0f), Alignment9::MiddleCenter, SizeF{152.0f, 4.0f}}
                }
                .setColor(ColorF32{0.15f})
                .pushAuto();

            DrawLabelText(ToUtf32(std::format("{}", aliveMachines)),
                          32.0f,
                          Screen::TopCenterF().movedY(112.0f),
                          Alignment9::TopCenter);
        }

        // -----------------------------------------------
        // ラップタイム
        DrawLabelText(
            ToUtf32("Lap {} / {}",
                    Min<int>(m_measuredLapTimes.size(), player.state.m_markedLapProgress.lapIndex + 1),
                    m_measuredLapTimes.size()),
            24.0f,
            Screen::TopRightF().movedBy(-20.0f, 40.0f),
            Alignment9::TopRight);

        for (int i = 0; i < m_measuredLapTimes.size(); ++i)
        {
            if (i > player.state.m_markedLapProgress.lapIndex)
            {
                break;
            }

            const bool isCurrentLap = (i == player.state.m_markedLapProgress.lapIndex);

            const float lapTime = m_measuredLapTimes[i];
            DrawLabelText(
                ToUtf32(formatLapTime(lapTime) + " "), // TODO: エンジン側のバグ調査 (末尾に + " " 入れると安定する)
                24.0f,
                Screen::TopRightF().movedBy(-20.0f, 80.0f + i * 32.0f),
                Alignment9::TopRight,
                isCurrentLap ? std::optional<ColorF32>{Palette::Orange} : std::nullopt);
        }

        // -----------------------------------------------

        drawSpecialUI();

        if (IsGameStatsVisible()) // FIXME
        {
            drawTutorial();
        }

        drawBoostTutorialIfNeeded();

        // -----------------------------------------------

        ImmediateDrawer::Global().draw();
    }

    void drawSpecialUI() const
    {
        if (m_countdown > 0)
        {
            DrawSpecialLabelText(
                ToUtf32("- {} -", m_countdown),
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            return;
        }

        if (m_majorBanner == MajorBanner::Go)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            return;
        }
        else if (m_majorBanner == MajorBanner::YouGotBoostPower)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
            DrawSpecialLabelText(
                U"Lap 2 / 3",
                32.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter);
            return;
        }
        else if (m_majorBanner == MajorBanner::TheFinalLap)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
            DrawSpecialLabelText(
                U"Lap 3 / 3",
                32.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter);
            return;
        }
        else if (m_majorBanner == MajorBanner::Finish)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);

            if (m_playerFinalRank != -1)
            {
                DrawSpecialLabelText(
                    ToUtf32("{} / {}", m_playerFinalRank + 1, GetRaceContext().machineManager().machineList().size()),
                    96.0f,
                    Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            }

            return;
        }
        else if (m_majorBanner == MajorBanner::YourMachineHasCrashed)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter,
                Palette::Red);
            return;
        }

        const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
        if (player.state.m_isFallingOffCourse)
        {
            DrawSpecialLabelText(
                U"Fall Off the Track !",
                64.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter,
                Palette::Orange);
            return;
        }

        if (m_reverseDirectionChecker.IsReversing())
        {
            DrawSpecialLabelText(
                U"Reverse Course !",
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter,
                Palette::Orange);
            Immediate2D_Text::MPlus1_Sdf(U"\U000F17B0")
                .setSize(256.0f)
                .setPosition(Screen::RectF().middleCenter(), Alignment9::MiddleCenter)
                .setColor(Palette::Orange)
                .pushAuto();
            return;
        }
    }

    void drawTutorial() const
    {
        Array<std::u32string> messages{};

        std::u32string boostMessage =
            GetRaceContext().machineManager().machineList()[PlayerMachineId].state.isBoostUnlocked()
                ? U"ブースト"
                : U"?";
        if (IsUsingGamepad())
        {
            messages.push_back(U"[ 左スティック ]: 横移動");
            messages.push_back(U"[ A ]: アクセル");
            messages.push_back(U"[ B ]: " + boostMessage);
            messages.push_back(U"[ L ]: 左ドリフト | [ R ]: 右ドリフト");
        }
        else
        {
            messages.push_back(U"[ A ], [ D ]: 横移動");
            messages.push_back(U"[ Shift ]: アクセル");
            messages.push_back(U"[ Space ]: " + boostMessage);
            messages.push_back(U"[ 左矢印 ]: 左ドリフト | [ 右矢印 ]: 右ドリフト");
        }

        for (int i = 0; i < messages.size(); ++i)
        {
            Immediate2D_Text::MPlus1_Sdf(messages[i])
                .setSize(16.0f)
                .setPosition({80.0f, 80.0f + i * 24.0f})
                .pushAuto();
        }
    }

    void drawBoostTutorialIfNeeded() const
    {
        if (not m_boostTutorialEnabled)
        {
            return;
        }

        if (IsUsingGamepad())
        {
            auto text =
                Immediate2D_Text::MPlus1_Sdf(U"[ B ] でブースト")
                .setSize(24.0f)
                .setPosition(Screen::RectF().getRelativePoint({0.5f, 0.375f}), Alignment9::MiddleCenter)
                .setColor(ColorF32{0.3f})
                .cache();
            Immediate2D::RoundRect{text.region.stretched(32.0f, 4.0f)}
                .setColor(GamePalette::GamingGreen)
                .pushAuto();
            text.pushAuto();
        }
    }

    void killed() override
    {
        m_children.killEach();

        Audio::StopAllSounds();
        Audio::StopMusic();

        GetRaceContext().unregisterDrawer(this);
    }
};

namespace Race
{
    RaceFlowController::RaceFlowController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceFlowController::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> RaceFlowController::asActor() const
    {
        return p_impl;
    }
}
