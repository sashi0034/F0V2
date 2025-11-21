#pragma once

namespace Race
{
    enum class CourseGimmickKind : uint8_t
    {
        Barrier,
        BoostPad_C,
        JumpPad_C,
        PitZone_L,
        PitZone_C,
        PitZone_R,
        PitZone_LR,
        Max // end marker
    };

    struct GimmickTriangleAttribute
    {
        enum class kind_t : uint8_t
        {
            Barrier,
            BoostPad,
            JumpPad,
            PitZone,
        };

        kind_t kind;
    };
}
