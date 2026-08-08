#pragma once
#include "TY/Alignment.h"
#include "TY/Color.h"
#include "TY/Vector2D.h"

namespace Race
{
    constexpr ColorF32 LabelTextBackground = ColorF32{0.15f};

    void DrawLabelText(
        const std::u32string& text,
        float size,
        const Float2& pos,
        Alignment9 alignment,
        const std::optional<ColorF32>& colorOpt = std::nullopt,
        const std::optional<ColorF32>& backgroundOpt = std::nullopt);

    void DrawSpecialLabelText(
        const std::u32string& text,
        float size,
        const Float2& pos,
        Alignment9 alignment,
        const std::optional<ColorF32>& colorOpt = std::nullopt,
        const std::optional<ColorF32>& backgroundOpt = std::nullopt);
}
