#include "pch.h"
#include "Hud_DurabilityBar.h"

#include "Hud_LabelText.h"
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

struct Hud_DurabilityBar::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"Hud_DurabilityBar";
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
        const Float2 bottomLeft = Screen::RectF().bl().movedBy(40.0f, -160.0f);
        constexpr SizeF barSize{320.0f, 12.0f};
        Immediate2D::RoundRect{RectF{bottomLeft, Alignment9::BottomLeft, barSize}}
            .setColor(ColorF32{0.2f})
            .pushAuto();

        const float displayRate = Math::Clamp(m_displayDurability / m_actualMaxDurability, 0.0f, 1.0f);
        const Float2 displaySize = barSize.withX(barSize.x * displayRate);
        Immediate2D::RoundRect{
                RectF{bottomLeft, Alignment9::BottomLeft, displaySize}
                .stretched(-Min(4.0f, displaySize.x * 0.5f), -1.0f)
            }
            .setColor(Palette::Crimson)
            .pushAuto();

        const float actualRate = Math::Clamp(m_actualDurability / m_actualMaxDurability, 0.0f, 1.0f);
        const Float2 actualSize = barSize.withX(barSize.x * actualRate);
        Immediate2D::RoundRect{
                RectF{bottomLeft, Alignment9::BottomLeft, actualSize}
                .stretched(-Min(4.0f, actualSize.x * 0.5f), -1.0f)
            }
            .setColor(Palette::CornflowerBlue)
            .pushAuto();

        const auto labelColor =
            actualSize.x + 1.0f < displaySize.x ? std::optional(Palette::Crimson) : std::nullopt;
        DrawLabelText(ToUtf32("{}", static_cast<int>(m_actualDurability)),
                      20.0f,
                      bottomLeft.movedBy(barSize.x, -barSize.y - 4.0),
                      Alignment9::BottomRight,
                      labelColor);
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
    Hud_DurabilityBar::Hud_DurabilityBar() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void Hud_DurabilityBar::init()
    {
        p_impl->Init();
    }

    void Hud_DurabilityBar::draw() const
    {
        p_impl->Draw();
    }

    std::shared_ptr<ActorBase> Hud_DurabilityBar::asActor() const
    {
        return p_impl;
    }
}
