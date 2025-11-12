#include "pch.h"
#include "Hud_LabelText.h"

#include "Asset0.h"
#include "GamePalette.h"
#include "TY/Palette.h"

namespace Race
{
    void DrawLabelText(
        const std::u32string& text,
        float size,
        const Float2& pos,
        Alignment9 alignment,
        const std::optional<ColorF32>& colorOpt)
    {
        auto t = Immediate2D_Text::Audiowide_Sdf(text)
                 .setSize(size)
                 .setPosition(pos, alignment)
                 .setColor(colorOpt.value_or(Palette::LightSteelBlue))
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

    void DrawSpecialLabelText(const std::u32string& text, float size, const Float2& pos, Alignment9 alignment)
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
}
