#include "pch.h"
#include "RaceUIController.h"

#include "Asset0.h"
#include "GameGlobalUI.h"
#include "GamePalette.h"
#include "Race/IRaceContext.h"
#include "Race/IRaceDrawer.h"
#include "Race/RaceControlState.h"
#include "UI_DurabilityBar.h"
#include "UI_LabelText.h"
#include "TY/ActorContainer.h"
#include "TY/Gamepad.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Palette.h"
#include "TY/Periodic.h"
#include "TY/Screen.h"
#include "TY/Utils.h"

using namespace Race;

namespace
{
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
}

struct RaceUIController::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"RaceUIController";
#endif
    ActorContainer m_children{};
    const RaceControlState* m_raceControlState{};
    UI_DurabilityBar m_durabilityBar{};

    void Init(const RaceControlState& raceControlState)
    {
        m_raceControlState = &raceControlState;

        GetRaceContext().registerDrawer(shared_from_this());

        m_durabilityBar = m_children.birth(UI_DurabilityBar());
        m_durabilityBar.init();
    }

private:
    void update() override
    {
        m_children.updateEach();
    }

    void drawUI() const override
    {
        const auto& state = *m_raceControlState;
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
        // ブーストコンボ
        if (player.state.m_boostComboCountdown > 0.0f || player.state.m_manualBoostCooldownTime > 0.0f)
        {
            const auto color = Palette::Orange;
            // player.state.m_manualBoostCooldownTime > 0.0f
            //     ? Palette::Orange
            //     : GamePalette::GamingGreen;

            // 文字が荒ぶってるけどコンボチャンス時だけ停止
            Float2 offset{};
            if (player.state.m_manualBoostCooldownTime > 0.0f)
            {
                // TODO: 最初はもっと荒ぶらせる
                const float noiseTable[] = {0, 2, 4, 1, 3,};
                const float noise = noiseTable[(static_cast<int>(System::Time() * 1000) / 50) % std::size(noiseTable)];
                offset = Float2::FromAngle(noise * (Math::TwoPiF / std::size(noiseTable))) * 4.0f;
            }

            DrawLabelText(
                ToUtf32(player.state.m_boostComboCount == 0 ? "Boost" : "Boost Combo"),
                24.0f,
                Screen::BottomCenterF().movedBy(offset.movedY(-96.0f)),
                Alignment9::BottomCenter,
                color);

            std::u32string comboMessage{};
            if (player.state.m_boostComboCount > 0)
            {
                comboMessage = ToUtf32(std::format("{} Combo", player.state.m_boostComboCount));
            }

            if (player.state.m_manualBoostCooldownTime <= 0.0f)
            {
                // TODO: 文字がちょっとずつ出るアニメーション
                comboMessage = U"Combo Chance !";
            }

            if (not comboMessage.empty())
            {
                DrawLabelText(
                    comboMessage,
                    32.0f,
                    Screen::BottomCenterF().movedBy(offset.movedY(-80.0f)),
                    Alignment9::TopCenter,
                    color);
            }
        }

        // -----------------------------------------------
        // 順位

        if (not state.m_raceFinished)
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
                    Min<int>(state.m_measuredLapTimes.size(), player.state.m_markedLapProgress.lapIndex + 1),
                    state.m_measuredLapTimes.size()),
            24.0f,
            Screen::TopRightF().movedBy(-20.0f, 40.0f),
            Alignment9::TopRight);

        for (int i = 0; i < state.m_measuredLapTimes.size(); ++i)
        {
            if (i > player.state.m_markedLapProgress.lapIndex)
            {
                break;
            }

            const bool isCurrentLap = (i == player.state.m_markedLapProgress.lapIndex);
            const float lapTime = state.m_measuredLapTimes[i];
            DrawLabelText(
                ToUtf32(formatLapTime(lapTime)),
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
        const auto& state = *m_raceControlState;

        if (state.m_startCountdown > 0)
        {
            DrawSpecialLabelText(
                ToUtf32("- {} -", state.m_startCountdown),
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            return;
        }

        if (state.m_centerBanner == RaceCenterBanner::Go)
        {
            DrawSpecialLabelText(
                state.m_centerBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            return;
        }
        else if (state.m_centerBanner == RaceCenterBanner::YouGotBoostPower)
        {
            DrawSpecialLabelText(
                state.m_centerBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
            DrawSpecialLabelText(
                U"Lap 2 / 3",
                32.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter);
            return;
        }
        else if (state.m_centerBanner == RaceCenterBanner::TheFinalLap)
        {
            DrawSpecialLabelText(
                state.m_centerBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
            DrawSpecialLabelText(
                U"Lap 3 / 3",
                32.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter);
            return;
        }
        else if (state.m_centerBanner == RaceCenterBanner::Finish)
        {
            DrawSpecialLabelText(
                state.m_centerBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);

            if (state.m_playerFinalRank != -1)
            {
                DrawSpecialLabelText(
                    ToUtf32("{} / {}", state.m_playerFinalRank + 1,
                            GetRaceContext().machineManager().machineList().size()),
                    96.0f,
                    Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            }
            return;
        }
        else if (state.m_centerBanner == RaceCenterBanner::YourMachineHasCrashed)
        {
            DrawSpecialLabelText(
                state.m_centerBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter,
                Palette::Red);
            return;
        }

        if (GetRaceContext().machineManager().machineList()[PlayerMachineId].state.m_isFallingOffCourse)
        {
            DrawSpecialLabelText(
                U"Fall Off the Track !",
                64.0f,
                Screen::RectF().middleCenter(), Alignment9::MiddleCenter,
                Palette::Orange);
            return;
        }

        if (state.m_isReversing)
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
        if (not m_raceControlState->m_boostTutorialEnabled)
        {
            return;
        }

        auto text =
            Immediate2D_Text::MPlus1_Sdf(ToUtf32(
                "[ {} ] でブースト",
                IsUsingGamepad() ? "B" : "Space"))
            .setSize(24.0f)
            .setPosition(Screen::RectF().getRelativePoint({0.5f, 0.375f}), Alignment9::MiddleCenter)
            .setColor(ColorF32{0.3f})
            .cache();
        Immediate2D::RoundRect{text.region.stretched(32.0f, 4.0f)}
            .setColor(GamePalette::GamingGreen)
            .pushAuto();
        text.pushAuto();
    }

    void killed() override
    {
        m_children.killEach();
        GetRaceContext().unregisterDrawer(this);
    }
};

namespace Race
{
    RaceUIController::RaceUIController() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceUIController::init(const RaceControlState& raceControlState)
    {
        p_impl->Init(raceControlState);
    }

    std::shared_ptr<ActorBase> RaceUIController::asActor() const
    {
        return p_impl;
    }
}
