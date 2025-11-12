#include "pch.h"
#include "RaceFlowController.h"

#include "IRaceContext.h"
#include "IRaceDrawer.h"
#include "Common/RaceSharedState.h"
#include "Hud/Hud_DurabilityBar.h"
#include "Hud/Hud_LabelText.h"
#include "TY/ActorContainer.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "Util/DebugTomlValue.h"

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
}

struct RaceFlowController::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceFlowController"};
#endif

    ActorContainer m_children{};

    Hud_DurabilityBar m_durabilityBar{};

    CoroutineActor m_raceFlowCoroutine{};

    int m_countdown{};

    MajorBanner m_majorBanner{MajorBanner::None};

    std::u32string m_majorBannerMessage{};

    CoroutineActor m_majorBannerCoroutine{};

    int m_playerFinalRank{-1};

    bool m_raceFinished{};
    bool m_playerCrashed{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        m_raceFlowCoroutine = StartCoroutine(m_children, [this](AwaitContext& await)
        {
            processRaceFlow(await);
        });

        m_durabilityBar = m_children.birth(Hud_DurabilityBar());
        m_durabilityBar.init();
    }

private:
    void update() override
    {
        m_children.updateEach();

        // ゲームオーバーチェック
        const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
        if (not m_raceFinished && player.state.isDead() && not m_playerCrashed)
        {
            m_raceFlowCoroutine.kill();

            m_playerCrashed = true;

            popupMajorBanner(MajorBanner::YourMachineHasCrashed, U"Your Machine Has Crashed !", -1);
        }
    }

    void processRaceFlow(AwaitContext& await)
    {
        g_sharedState->isRaceStarted = false;

        await.waitForFrames(1);

        process321Go(await);

        g_sharedState->isRaceStarted = true;

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 1;
        });

        popupMajorBanner(MajorBanner::YouGotBoostPower, U"You've Got Boost Power !", 5.0f);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 2;
        });

        popupMajorBanner(MajorBanner::TheFinalLap, U"The Final Lap !", 5.0f);

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 3;
        });

        popupMajorBanner(MajorBanner::Finish, U"Finish !", -1);

        m_raceFinished = true;

        const int playerRank = GetRaceContext().machineManager().getEvaluation(PlayerMachineId).rank;

        await.waitForTime(2.5s);

        m_playerFinalRank = playerRank;
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

        await.waitForTime(1.0f);

        m_countdown--;

        await.waitForTime(1.0f);

        m_countdown--;

        await.waitForTime(1.0f);

        m_countdown--;

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

    void drawHud() const override
    {
        const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];

        // スピードメーター
        DrawLabelText(ToUtf32(std::format("{:.0f} km/h", player.state.m_velocity.length() * 10.0f)),
                      28.0f,
                      Screen::SizeF().movedBy(-20.0f, -12.0f),

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
            const int totalMachines = GetRaceContext().machineManager().machineList().size();
            DrawLabelText(ToUtf32(std::format("{}", rank1)),
                          64.0f,
                          Screen::TopCenterF().movedY(40.0f),
                          Alignment9::TopCenter);

            Immediate2D::Rect{
                    RectF{Screen::TopCenterF().movedY(110.0f), Alignment9::MiddleCenter, SizeF{152.0f, 4.0f}}
                }
                .setColor(ColorF32{0.15f})
                .pushAuto();

            DrawLabelText(ToUtf32(std::format("{}", totalMachines)),
                          32.0f,
                          Screen::TopCenterF().movedY(112.0f),
                          Alignment9::TopCenter);
        }

        drawSpecialHud();

        // -----------------------------------------------

        ImmediateDrawer::Global().draw();
    }

    void drawSpecialHud() const
    {
        if (m_countdown > 0)
        {
            DrawSpecialLabelText(
                ToUtf32("- {} -", m_countdown),
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
        }

        if (m_majorBanner == MajorBanner::Go)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
        }
        else if (m_majorBanner == MajorBanner::YouGotBoostPower)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
        }
        else if (m_majorBanner == MajorBanner::TheFinalLap)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
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
        }
        else if (m_majorBanner == MajorBanner::YourMachineHasCrashed)
        {
            DrawSpecialLabelText(
                m_majorBannerMessage,
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter,
                Palette::Red);
        }
    }

    void killed() override
    {
        m_children.killEach();

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
