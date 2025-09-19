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
