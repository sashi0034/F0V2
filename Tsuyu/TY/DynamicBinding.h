#pragma once
#include <cstddef>
#include <cstdint>

#include "DynamicHandle.h"
#include "RootParameterIndex.h"

namespace TY
{
    namespace DynamicBinding
    {
        DynamicCbvHandle UploadDynamicCbv(const void* data, size_t size);

        template <typename T>
        DynamicCbvHandle UploadDynamicCbv(const T& data)
        {
            return UploadDynamicCbv(&data, sizeof(T));
        }

        void SetDynamicCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size);

        template <typename T>
        void SetDynamicCbv(RootParameterIndex rootParameterIndex, const T& data)
        {
            return SetDynamicCbv(rootParameterIndex, &data, sizeof(T));
        }

        void SetDynamicCbv(RootParameterIndex rootParameterIndex, DynamicCbvHandle cbv);

        void FlushAsGraphics();
    }
}
