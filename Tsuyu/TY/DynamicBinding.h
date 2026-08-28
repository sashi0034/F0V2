#pragma once
#include <cstddef>
#include <cstdint>

#include "RootParameterIndex.h"

namespace TY
{
    namespace DynamicBinding
    {
        using GpuAddress = uint64_t;

        GpuAddress UploadDynamicCbv(const void* data, size_t size);

        template <typename T>
        GpuAddress UploadDynamicCbv(const T& data)
        {
            return UploadDynamicCbv(&data, sizeof(T));
        }

        GpuAddress SetDynamicCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size);

        template <typename T>
        GpuAddress SetDynamicCbv(RootParameterIndex rootParameterIndex, const T& data)
        {
            return SetDynamicCbv(rootParameterIndex, &data, sizeof(T));
        }

        void SetDynamicCbvByAddress(RootParameterIndex rootParameterIndex, GpuAddress address);

        void FlushAsGraphics();
    }
}
