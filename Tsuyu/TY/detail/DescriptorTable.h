#pragma once
#include "TY/Array.h"

namespace TY::detail
{
    struct DescriptorTableElement
    {
        uint32_t cbvCount{};
        uint32_t srvCount{};
        uint32_t uavCount{};

        constexpr DescriptorTableElement(uint32_t cbvCount, uint32_t srvCount, uint32_t uavCount) :
            cbvCount(cbvCount),
            srvCount(srvCount),
            uavCount(uavCount)
        {
        }
    };

    using DescriptorTable = Array<DescriptorTableElement>;
}
