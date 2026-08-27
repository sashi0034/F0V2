#pragma once

namespace TY::detail
{
    struct BindingSlot
    {
        int cbvStart;
        int srvStart;
        int uavStart;

        bool operator==(const BindingSlot& other) const = default;
    };
}
