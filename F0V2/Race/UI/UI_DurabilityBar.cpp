#include "pch.h"
#include "UI_DurabilityBar.h"

#include "GamePalette.h"
#include "UI_LabelText.h"
#include "Race/IRaceContext.h"
#include "Race/Machine/MachineConstants.h"
#include "TY/ActorContainer.h"
#include "TY/ActorLifetimeScope.h"
#include "TY/EaseActor.h"
#include "TY/Easing.h"
#include "TY/Immediate2D.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "TY/Utils.h"
#include "TY_Extension/AwaitContext.h"
#include "TY_Extension/CoroutineActor.h"

using namespace Race;

namespace
{
}

struct UI_DurabilityBar::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"UI_DurabilityBar";
#endif
    ActorContainer m_children{};

    ActorLifetimeScope m_animation{};

    float m_actualDurability{};
    float m_actualMaxDurability{};

    float m_displayDurability{};

    void Init()
    {
    }

    void Draw() const
    {
        const Float2 bottomLeft = Screen::RectF().bl().movedBy(40.0f, -120.0f);
        constexpr SizeF barSize{600.0f, 12.0f};
        Immediate2D::RoundRect{RectF{bottomLeft, Alignment9::BottomLeft, barSize}}
            .setColor(ColorF32{0.2f})
            .pushAuto();

        Float2 displaySize, actualSize;
        if (m_actualDurability < m_displayDurability)
        {
            displaySize = drawBar(m_displayDurability, Palette::Crimson, bottomLeft, barSize);
            actualSize = drawBar(m_actualDurability, Palette::CornflowerBlue, bottomLeft, barSize);
        }
        else if (m_displayDurability < m_actualDurability)
        {
            actualSize = drawBar(m_actualDurability, Palette::White, bottomLeft, barSize);
            displaySize = drawBar(m_displayDurability, Palette::CornflowerBlue, bottomLeft, barSize);
        }
        else
        {
            actualSize = drawBar(m_actualDurability, Palette::CornflowerBlue, bottomLeft, barSize);
            displaySize = actualSize;
        }

        const auto labelColor =
            actualSize.x + 1.0f < displaySize.x || m_displayDurability == 0.0f
                ? std::optional(Palette::Crimson)
                : std::nullopt;
        DrawLabelText(ToUtf32("{}", static_cast<int>(m_actualDurability)),
                      20.0f,
                      bottomLeft.movedBy(barSize.x, -barSize.y - 4.0),
                      Alignment9::BottomRight,
                      labelColor);

        DrawLabelText(U"Energy",
                      20.0f,
                      bottomLeft.movedY(-barSize.y - 4.0),
                      Alignment9::BottomLeft,
                      std::nullopt);

        const auto& machine = GetRaceContext().machineManager().fetchMachine(PlayerMachineId);
        if (machine.state.isBoostUnlocked())
        {
            DrawLabelText(
                U"Boost Power",
                20.0f,
                bottomLeft.movedX(barSize.y + 4.0f),
                Alignment9::TopLeft,
                GamePalette::GamingGreen);
        }
    }

    SizeF drawBar(float durability, const ColorF32& color, const Float2& bottomLeft, const Float2& barSize) const
    {
        const float rate = Math::Clamp(durability / m_actualMaxDurability, 0.0f, 1.0f);
        const Float2 size = barSize.withX(barSize.x * rate);
        Immediate2D::RoundRect{
                RectF{bottomLeft, Alignment9::BottomLeft, size}
                .stretched(-Min(4.0f, size.x * 0.5f), -1.0f)
            }
            .setColor(color)
            .pushAuto();
        return size;
    }

private:
    void update() override
    {
        m_children.updateEach();

        const auto& machine = GetRaceContext().machineManager().fetchMachine(PlayerMachineId);

        if (m_actualMaxDurability == 0.0f)
        {
            m_actualDurability = machine.state.m_durability;
            m_actualMaxDurability = machine.props.maxDurability;
            m_displayDurability = m_actualDurability;
            return;
        }

        if (m_actualDurability != machine.state.m_durability)
        {
            m_actualDurability = machine.state.m_durability;

            m_animation.clear();
            StartCoroutine(m_children, [this](AwaitContext& await)
            {
                await.waitForTime(0.5s);

                StartEasing<EaseOutExpo>(m_children, m_displayDurability, m_actualDurability, 0.5s)
                    >> m_animation;
            }) >> m_animation;
        }
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    UI_DurabilityBar::UI_DurabilityBar() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void UI_DurabilityBar::init()
    {
        p_impl->Init();
    }

    void UI_DurabilityBar::draw() const
    {
        p_impl->Draw();
    }

    std::shared_ptr<ActorBase> UI_DurabilityBar::asActor() const
    {
        return p_impl;
    }
}
