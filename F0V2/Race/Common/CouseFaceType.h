#pragma once
#include <cstdint>

namespace Race
{
    /// @brief gbuffer_course*.hlsl で定義されている FaceType
    enum class CourseFaceType : uint8_t
    {
        Default, // テクスチャをそのまま出す面

        RoadTop,
        RoadBottom,
        RoadSide,

        PipeEntryExitTop,
        PipeEntryExitBottom,
        PipeEntryExitSide,
        PipeInner,
        PipeOuter,
        PipeCap,

        CylinderEntryExitTop,
        CylinderEntryExitBottom,
        CylinderEntryExitSide,
        CylinderEntryExitCap,
        CylinderOuter,

        BarrierTop,
        BarrierSide,
        BarrierBottom,
    };
}
