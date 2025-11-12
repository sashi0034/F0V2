#include "pch.h"
#include "RaceFlowController.h"

#include "Asset0.h"
#include "GamePalette.h"
#include "IRaceContext.h"
#include "IRaceDrawer.h"
#include "Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "Util/ImmediatePrint.h"

using namespace Race;

namespace
{
    void drawLabelText(const std::u32string& text, float size, const Float2& pos, Alignment9 alignment)
    {
        auto t = Immediate2D_Text::Audiowide_Sdf(text)
                 .setSize(size)
                 .setPosition(pos, alignment)
                 .setColor(Palette::LightSteelBlue)
                 .cache();

        Immediate2D::RoundRect{t.region.stretched(8.0f, -4.0f)}
            .setColor(ColorF32{0.15f})
            .pushAuto();

        for (int i = 0; i < t.characters.size(); ++i)
        {
            if (text[i] == U' ')
            {
                continue;
            }

            Immediate2D::RoundRect{t.characters[i].rect()}
                .setColor(ColorF32{0.15f})
                .pushAuto();
        }

        t.pushAuto();
    }

    void drawSpecialText(const std::u32string& text, float size, const Float2& pos, Alignment9 alignment)
    {
        auto t = Immediate2D_Text::Audiowide_Sdf(text)
                 .setSize(size)
                 .setPosition(pos, alignment)
                 .setColor(GamePalette::GamingGreen)
                 .cache();

        Immediate2D::RoundRect{
                RectF{t.region.center(), Alignment9::MiddleCenter, SizeF{t.region.w + 64.0f, size * 0.75f}}
            }
            .setColor(ColorF32{0.15f})
            .setRoundness(40.0f)
            .pushAuto();

        // Immediate2D::Path()
        //     .append(t.region.middleLeft().movedX(-40.0f))
        //     .append(t.region.topCenter().movedY(-20.0f))
        //     .append(t.region.middleRight().movedX(40.0f))
        //     .append(t.region.bottomCenter().movedY(20.0f))
        //     .setThickness(40.0f)
        //     .setColor(ColorF32{0.15f})
        //     .asCycle()
        //     .pushAuto();

        for (int i = 0; i < t.characters.size(); ++i)
        {
            if (text[i] == U' ')
            {
                continue;
            }

            Immediate2D::RoundRect{t.characters[i].rect()}
                .setColor(ColorF32{0.15f})
                .setRoundness(20.0f)
                .pushAuto();
        }

        t.pushAuto();
    }

    enum class MajorBanner : uint8_t
    {
        None,
        Go,
        YouGotBoostPower,
        TheFinalLap,
        Finish,
    };
}

struct RaceFlowController::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceFlowController"};
#endif

    ActorContainer m_children{};

    int m_countdown{};

    MajorBanner m_majorBanner{MajorBanner::None};

    int m_playerFinalRank{-1};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        StartCoroutine(m_children, [this](AwaitContext& await)
        {
            runRaceFlow(await);
        });
    }

private:
    void update() override
    {
        m_children.updateEach();
    }

    void runRaceFlow(AwaitContext& await)
    {
        g_sharedState->isRaceStarted = false;

        m_countdown = 3;

        await.waitForTime(1.0f);

        m_countdown--;

        await.waitForTime(1.0f);

        m_countdown--;

        await.waitForTime(1.0f);

        m_countdown--;

        g_sharedState->isRaceStarted = true;

        popupMajorBanner(MajorBanner::Go, 3.0f) >> await.lifetime();

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 1;
        });

        popupMajorBanner(MajorBanner::YouGotBoostPower, 5.0f) >> await.lifetime();

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 2;
        });

        popupMajorBanner(MajorBanner::TheFinalLap, 5.0f) >> await.lifetime();

        // -----------------------------------------------

        await.waitForTrue([&]()
        {
            const auto& player = GetRaceContext().machineManager().machineList()[PlayerMachineId];
            return player.state.m_lapProgress.lapIndex == 3;
        });

        m_majorBanner = MajorBanner::Finish;

        const int playerRank = GetRaceContext().machineManager().getEvaluation(PlayerMachineId).rank;

        await.waitForTime(2.5s);

        m_playerFinalRank = playerRank;
    }

    CoroutineActor popupMajorBanner(MajorBanner banner, float seconds)
    {
        m_majorBanner = banner;
        return StartCoroutine(m_children, [this, banner, seconds](AwaitContext& await)
        {
            await.waitForTime(seconds);

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
        drawLabelText(ToUtf32(std::format("{:.0f} km/h", player.state.m_velocity.length() * 10.0f)),
                      28.0f,
                      Screen::SizeF().movedBy(-20.0f, -12.0f),

                      Alignment9::BottomRight);

        // -----------------------------------------------
        // 耐久値バー
        {
            const float barRate = Math::Clamp(player.state.m_durability / player.props.maxDurability, 0.0f, 1.0f);
            const Float2 bottomLeft = Screen::RectF().bl().movedBy(40.0f, -160.0f);
            constexpr SizeF barSize{320.0f, 12.0f};
            Immediate2D::RoundRect{RectF{bottomLeft, Alignment9::BottomLeft, barSize}}
                .setColor(ColorF32{0.2f})
                .pushAuto();
            Immediate2D::RoundRect{
                    RectF{bottomLeft, Alignment9::BottomLeft, barSize.withX(barSize.x * barRate)}
                    .stretched(-4.0f, -1.0f)
                }
                .setColor(Palette::CornflowerBlue)
                .pushAuto();
            drawLabelText(ToUtf32("{}", static_cast<int>(player.state.m_durability)),
                          20.0f,
                          bottomLeft.movedBy(barSize.x, -barSize.y - 4.0),
                          Alignment9::BottomRight);
        }

        // -----------------------------------------------
        // 順位

        if (m_majorBanner != MajorBanner::Finish)
        {
            const auto evaluation = GetRaceContext().machineManager().getEvaluation(PlayerMachineId);
            const int rank1 = evaluation.rank + 1;
            const int totalMachines = GetRaceContext().machineManager().machineList().size();
            drawLabelText(ToUtf32(std::format("{}", rank1)),
                          64.0f,
                          Screen::TopCenterF().movedY(40.0f),
                          Alignment9::TopCenter);

            Immediate2D::Rect{
                    RectF{Screen::TopCenterF().movedY(110.0f), Alignment9::MiddleCenter, SizeF{152.0f, 4.0f}}
                }
                .setColor(ColorF32{0.15f})
                .pushAuto();

            drawLabelText(ToUtf32(std::format("{}", totalMachines)),
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
            drawSpecialText(
                ToUtf32("- {} -", m_countdown),
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
        }

        if (m_majorBanner == MajorBanner::Go)
        {
            drawSpecialText(
                ToUtf32("Go !", m_countdown),
                96.0f,
                Screen::MiddleCenterF(), Alignment9::MiddleCenter);
        }
        else if (m_majorBanner == MajorBanner::YouGotBoostPower)
        {
            drawSpecialText(
                ToUtf32("You've Got Boost Power !"),
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
        }
        else if (m_majorBanner == MajorBanner::TheFinalLap)
        {
            drawSpecialText(
                ToUtf32("The Final Lap !"),
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);
        }
        else if (m_majorBanner == MajorBanner::Finish)
        {
            drawSpecialText(
                ToUtf32("Finish !"),
                64.0f,
                Screen::RectF().getRelativePoint({0.5f, 0.25f}), Alignment9::MiddleCenter);

            if (m_playerFinalRank != -1)
            {
                drawSpecialText(
                    ToUtf32("{} / {}", m_playerFinalRank + 1, GetRaceContext().machineManager().machineList().size()),
                    96.0f,
                    Screen::MiddleCenterF(), Alignment9::MiddleCenter);
            }
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
