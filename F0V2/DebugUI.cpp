#include "pch.h"
#include "DebugUI.h"

#include "Asset0.h"
#include "ColorPalette.h"
#include "TY/Intersects.h"
#include "TY/Mouse.h"

bool DebugUI::Button(const RectF& region, const std::u32string& text)
{
    const bool isHovered = Intersects(region, Mouse::PosF());

    Shape2D::RoundRect{region}
        .setColor(ColorPalette::DarkOrange * (isHovered ? (MouseL.pressed() ? 1.3f : 1.5f) : 1.0f))
        .pushAuto();
    Shape2D_Text::MPlus1_16_Bitmap(text)
        .setPosition(region.middleCenter(), Alignment9::MiddleCenter)
        .pushAuto();

    return isHovered && MouseL.down();
}

bool DebugUI::ListSlider(
    int& startIndex, int pageCapacity, int listCount, const RectF& sliderRegion, const RectF& scrollRegion)
{
    if (pageCapacity >= listCount)
    {
        startIndex = 0;
        return false;
    }

    if (Intersects(Mouse::PosF(), scrollRegion))
    {
        if (Mouse::Wheel() > 0.0f)
        {
            startIndex = Max(0, startIndex - 1);
        }
        else if (Mouse::Wheel() < 0.0f)
        {
            startIndex = Min(listCount - pageCapacity, startIndex + 1);
        }
    }

    const float pageRatio = static_cast<float>(pageCapacity) / static_cast<float>(listCount);

    const float startRatio = static_cast<float>(startIndex) / static_cast<float>(listCount);

    const RectF drawRect = RectF{
        sliderRegion.pos + Float2{0.0f, sliderRegion.h * startRatio},
        SizeF{sliderRegion.w, sliderRegion.h * pageRatio}
    };
    Shape2D::RoundRect{drawRect}
        .setColor(ColorF32{"#4F4F4F"})
        .pushAuto();

    return false;
}
