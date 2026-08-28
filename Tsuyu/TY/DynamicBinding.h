#pragma once
#include <cstddef>

#include "RootParameterIndex.h"

namespace TY
{
    namespace DynamicBinding
    {
        void SetDynamicCbv(RootParameterIndex rootParameterIndex, const void* data, size_t size);

        template <typename T>
        void SetDynamicCbv(RootParameterIndex rootParameterIndex, const T& data)
        {
            SetDynamicCbv(rootParameterIndex, &data, sizeof(T));
        }

        void FlushAsGraphics();
    }
}
