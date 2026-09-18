#pragma once

namespace Race
{
    /// @brief 実行時に描画しておくコース用テクスチャの種類
    enum class CourseRenderTextureKind : uint8_t
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

    constexpr int CourseRenderTextureCount = static_cast<int>(CourseRenderTextureKind::Max);
}
