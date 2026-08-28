#pragma once

namespace TY
{
    struct DynamicCbvHandle
    {
        uint64_t address;

        explicit DynamicCbvHandle(uint64_t address = 0) : address(address)
        {
        }
    };
}
