#pragma once
#include <cstdint>

namespace TY
{
    struct DynamicVertexBufferHandle
    {
        uint64_t address{};
        uint32_t sizeInBytes{};
        uint32_t strideInBytes{};

        DynamicVertexBufferHandle() = default;

        explicit DynamicVertexBufferHandle(uint64_t address, uint32_t sizeInBytes, uint32_t strideInBytes)
            : address(address), sizeInBytes(sizeInBytes), strideInBytes(strideInBytes)
        {
        }
    };

    struct DynamicIndexBufferHandle
    {
        uint64_t address{};
        uint32_t sizeInBytes{};

        DynamicIndexBufferHandle() = default;

        explicit DynamicIndexBufferHandle(uint64_t address, uint32_t sizeInBytes)
            : address(address), sizeInBytes(sizeInBytes)
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

    struct DynamicSrvHandle
    {
        uint64_t address{};

        explicit DynamicSrvHandle(uint64_t address = 0) : address(address)
        {
        }
    };
}
