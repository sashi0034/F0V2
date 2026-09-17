#pragma once

namespace Race
{
    /// @brief 実行時に描画しておくコース用テクスチャの種類
    enum class CourseDynamicTextureKind : uint8_t
    {
        RoadTop,
        RoadBottom,
        RoadSide,
        // TODO...

        BoostPad,
        JumpPad,
        PitZone,

        Max, // end marker
    };

    constexpr int CourseDynamicTextureKindCount = static_cast<int>(CourseDynamicTextureKind::Max);
}
