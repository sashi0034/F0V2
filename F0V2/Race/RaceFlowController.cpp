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
                RectF{t.region.center(), Alignment9::MiddleCenter, SizeF{t.region.w + 64.0f, 64.0f}}
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
}

struct RaceFlowController::Impl : ActorBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
#if defined(_DEBUG)
    std::string m_debugName{"RaceFlowController"};
#endif

    ActorContainer m_children{};

    int m_countdown{};

    bool m_showGo{};

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

        m_showGo = true;
        StartCoroutine(m_children, [this](AwaitContext& await_)
        {
            await_.waitForTime(3.0f);

            m_showGo = false;
        }) >> await.lifetime();
    }

    void drawHud() const override
    {
        const auto& machine = GetRaceContext().machineManager().machineList()[PlayerMachineId];

        // スピードメーター
        drawLabelText(ToUtf32(std::format("{:.0f} km/h", machine.state.m_velocity.length() * 10.0f)),
                      28.0f,
                      Screen::SizeF().movedBy(-20.0f, -12.0f),

                      Alignment9::BottomRight);

        // -----------------------------------------------
        // 耐久値バー
        {
            const float barRate = Math::Clamp(machine.state.m_durability / machine.props.maxDurability, 0.0f, 1.0f);
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
            drawLabelText(ToUtf32("{}", static_cast<int>(machine.state.m_durability)),
                          20.0f,
                          bottomLeft.movedBy(barSize.x, -barSize.y - 4.0),
                          Alignment9::BottomRight);
        }

        // -----------------------------------------------
        // 順位
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
                Screen::MiddleCenterF(),
                Alignment9::MiddleCenter);
        }

        if (m_showGo)
        {
            drawSpecialText(
                ToUtf32("Go !", m_countdown),
                96.0f,
                Screen::MiddleCenterF(),
                Alignment9::MiddleCenter);
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
