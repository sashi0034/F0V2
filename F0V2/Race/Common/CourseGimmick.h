#pragma once

namespace Race
{
    enum class CourseGimmickKind : uint8_t
    {
        Barrier,
        BoostPad_L,
        BoostPad_C,
        BoostPad_R,
        JumpPad_L,
        JumpPad_C,
        JumpPad_R,
        PitZone_L,
        PitZone_C,
        PitZone_R,
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

    using GimmickFlagBits = uint32_t;

    namespace GimmickFlag
    {
        enum : GimmickFlagBits
        {
            Barrier = 1 << 0,
            BoostPad = 1 << 1,
            JumpPad = 1 << 2,
            PitZone = 1 << 3,
        };
    }
}
