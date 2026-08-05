#pragma once

namespace Race
{
    enum class CourseGimmickKind : uint8_t
    {
        Barrier,
        BoostPad_C,
        // TODO: BoostPad_L,
        // TODO: BoostPad_R,
        JumpPad_C,
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
