#pragma once
#include "TY/Array.h"

namespace TY::detail
{
    struct DescriptorEntry
    {
        static constexpr int AutoSlot = -1;

        int cbvSlot{AutoSlot};
        int cbvCount{};

        int srvSlot{AutoSlot};
        int srvCount{};

        int uavSlot{AutoSlot};
        int uavCount{};

        bool operator==(const DescriptorEntry& other) const = default;
    };

    using DescriptorTable = Array<DescriptorEntry>;
}
