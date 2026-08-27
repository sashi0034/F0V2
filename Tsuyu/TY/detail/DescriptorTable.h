#pragma once
#include <optional>

#include "BindingSlot.h"
#include "TY/Array.h"

namespace TY::detail
{
    struct DescriptorTableElement
    {
        uint32_t cbvCount{};
        uint32_t srvCount{};
        uint32_t uavCount{};
        std::optional<BindingSlot> bindingSlot{};

        constexpr DescriptorTableElement(
            size_t cbvCount,
            size_t srvCount,
            size_t uavCount,
            std::optional<BindingSlot> bindingSlot = std::nullopt) :
            cbvCount(cbvCount),
            srvCount(srvCount),
            uavCount(uavCount),
            bindingSlot(bindingSlot)
        {
        }

        bool operator==(const DescriptorTableElement& other) const = default;
    };

    using DescriptorTable = Array<DescriptorTableElement>;
}
