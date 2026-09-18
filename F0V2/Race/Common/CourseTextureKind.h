#pragma once

namespace Race
{
    /// @brief 実行時に描画しておくコース用テクスチャの種類
    enum class CourseTextureKind : uint8_t
    {
        None,

        StartingLine,

        RoadTop,
        RoadBottom,
        RoadSide,
        // TODO...

        BoostPad,
        JumpPad,
        PitZone,

        Max, // end marker
    };

    constexpr int CourseTextureCount = static_cast<int>(CourseTextureKind::Max);
}
