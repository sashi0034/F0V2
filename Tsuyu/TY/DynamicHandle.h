#pragma once

namespace TY
{
    struct DynamicVertexBufferHandle
    {
        uint64_t address{};

        explicit DynamicVertexBufferHandle(uint64_t address = 0) : address(address)
        {
        }
    };

    struct DynamicIndexBufferHandle
    {
        uint64_t address{};

        explicit DynamicIndexBufferHandle(uint64_t address = 0) : address(address)
        {
        }
    };

    struct DynamicCbvHandle
    {
        uint64_t address{};

        explicit DynamicCbvHandle(uint64_t address = 0) : address(address)
        {
        }
    };
}
