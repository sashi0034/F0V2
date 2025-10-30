#include "pch.h"
#include "DebugUI.h"

#include "Asset0.h"
#include "GamePalette.h"
#include "TY/Intersects2D.h"
#include "TY/Mouse.h"
#include "TY/System.h"

namespace
{
    size_t hashRect(const RectF& r) noexcept
    {
        const auto floatHash = [](float f) -> std::size_t
        {
            uint32_t u;
            std::memcpy(&u, &f, sizeof(float));
            return std::hash<uint32_t>{}(u);
        };

        const auto hashCombine = [](std::size_t seed, std::size_t v)
        {
            return seed ^ (v + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
        };

        std::size_t seed = 0;
        seed = hashCombine(seed, floatHash(r.x));
        seed = hashCombine(seed, floatHash(r.y));
        seed = hashCombine(seed, floatHash(r.w));
        seed = hashCombine(seed, floatHash(r.h));
        return seed;
    }
}

bool DebugUI::Button(const RectF& region, const std::u32string& text)
{
    const bool isHovered = Intersects(region, Mouse::PosF());

    Immediate2D::RoundRect{region}
        .setColor(GamePalette::DarkOrange * (isHovered ? (MouseL.pressed() ? 1.3f : 1.5f) : 1.0f))
        .pushAuto();
    Immediate2D_Text::MPlus1_16_Bitmap(text)
        .setPosition(region.middleCenter(), Alignment9::MiddleCenter)
        .pushAuto();

    return isHovered && MouseL.down();
}

bool DebugUI::DragButton(const RectF& region, const std::u32string& text)
{
    const bool isHovered = Intersects(region, Mouse::PosF());

    struct state_type
    {
        size_t dragTimestamp;
    };

    static std::unordered_map<size_t, state_type> s_states{};

    auto& state = s_states[hashRect(region)];

    const bool dragged = state.dragTimestamp == System::FrameCount() - 1;
    bool dragging{};
    if (not dragged)
    {
        if (isHovered && MouseL.down())
        {
            state.dragTimestamp = System::FrameCount();
            dragging = true;
        }
    }
    else // dragging
    {
        if (MouseL.pressed())
        {
            state.dragTimestamp = System::FrameCount();
            dragging = true;
        }
    }

    Immediate2D::RoundRect{region}
        .setColor(GamePalette::DarkOrange * (dragging ? 1.5f : (isHovered ? 1.3f : 1.0f)))
        .pushAuto();
    Immediate2D_Text::MPlus1_16_Bitmap(text)
        .setPosition(region.middleCenter(), Alignment9::MiddleCenter)
        .pushAuto();

    return dragging;
}

bool DebugUI::ItemButton(const RectF& region, const std::u32string& text, bool active)
{
    bool hovered = false;
    if (Intersects(region, Mouse::PosF()))
    {
        hovered = true;
    }

    Immediate2D::RoundRect{region}
        .setColor(ColorF32{0.15} * (hovered ? (MouseL.pressed() ? 1.3f : 1.5f) : 1.0f))
        .setOutline({active ? 1.0f : 0.0f, GamePalette::GoldenYellow})
        .pushAuto();

    Immediate2D_Text::MPlus1_16_Bitmap(text)
        .setPosition(region.stretched(-10).middleLeft(), Alignment9::MiddleLeft)
        .pushAuto();

    return hovered && MouseL.down();
}

bool DebugUI::ListSlider(
    int& startIndex, int pageCapacity, int listCount, const RectF& sliderRegion, const RectF& scrollRegion)
{
    if (pageCapacity >= listCount)
    {
        startIndex = 0;
        return false;
    }

    struct state_type
    {
        size_t dragTimestamp;
        float dragOffsetInThumb;
    };

    static std::unordered_map<size_t, state_type> s_states{};

    auto& state = s_states[hashRect(sliderRegion)];

    if (Intersects(scrollRegion, Mouse::PosF()))
    {
        const int d = pageCapacity / 5;
        if (Mouse::Wheel() > 0.0f)
        {
            startIndex = Max(0, startIndex - d);
        }
        else if (Mouse::Wheel() < 0.0f)
        {
            startIndex = Min(listCount - pageCapacity, startIndex + d);
        }
    }

    const float pageRatio = static_cast<float>(pageCapacity) / static_cast<float>(listCount);

    const float startRatio = static_cast<float>(startIndex) / static_cast<float>(listCount);

    const RectF thumbRect = RectF{
        sliderRegion.pos + Float2{0.0f, sliderRegion.h * startRatio},
        SizeF{sliderRegion.w, sliderRegion.h * pageRatio}
    };

    const bool dragging = state.dragTimestamp == System::FrameCount() - 1;
    if (not dragging)
    {
        if (MouseL.pressed() && Intersects(sliderRegion, Mouse::PosF()))
        {
            if (not Intersects(thumbRect, Mouse::PosF()))
            {
                startIndex += Max(1, pageCapacity / 2) * (thumbRect.y < Mouse::PosF().y ? 1 : -1);
            }
            else
            {
                state.dragTimestamp = System::FrameCount();
                state.dragOffsetInThumb = Mouse::PosF().y - thumbRect.y;
            }
        }
    }
    else // dragging
    {
        if (MouseL.pressed())
        {
            state.dragTimestamp = System::FrameCount();
            const float y = Mouse::PosF().y - state.dragOffsetInThumb;
            const float rate = (y - sliderRegion.y) / (sliderRegion.h - thumbRect.h);
            startIndex = static_cast<int>(rate * (listCount - pageCapacity));
        }
    }

    startIndex = Math::Clamp(startIndex, 0, listCount - pageCapacity);

    Immediate2D::RoundRect{thumbRect}
        .setColor(ColorF32{"#4F4F4F"} * (dragging ? 1.5f : 1.0f))
        .pushAuto();

    return state.dragTimestamp == System::FrameCount();
}
