#pragma once
#include <cstdint>

#include "BindingSlot.h"

namespace TY::detail
{
    struct DynamicDescriptorTableElement
    {
        uint32_t cbvCount{};
        BindingSlot bindingSlot{-1, -1, -1};

        bool operator==(const DynamicDescriptorTableElement& other) const = default;
    };
}
