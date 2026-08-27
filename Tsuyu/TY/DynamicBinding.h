#pragma once
#include <cstddef>

#include "RootParameterIndex.h"

namespace TY
{
    namespace DynamicBinding
    {
        void SetDynamicConstantBuffer(RootParameterIndex rootParameterIndex, const void* data, size_t size);

        template <typename T>
        void SetDynamicConstantBuffer(RootParameterIndex rootParameterIndex, const T& data)
        {
            SetDynamicConstantBuffer(rootParameterIndex, &data, sizeof(T));
        }

        void FlushAsGraphics();
    }
}
