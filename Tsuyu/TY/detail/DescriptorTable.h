#pragma once
#include "TY/Array.h"

namespace TY::detail
{
    struct DescriptorTableElement
    {
        static constexpr int AutoSlot = -1;

        int cbvSlot{AutoSlot};
        int cbvCount{};

        int srvSlot{AutoSlot};
        int srvCount{};

        int uavSlot{AutoSlot};
        int uavCount{};

        bool operator==(const DescriptorTableElement& other) const = default;
    };

    using DescriptorTable = Array<DescriptorTableElement>;
}
