#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "Array.h"
#include "DynamicHandle.h"
#include "RootParameterIndex.h"

namespace TY
{
    namespace DynamicBinding
    {
        DynamicVertexBufferHandle UploadDynamicVertexBuffer(const void* data, size_t size);

        template <typename T, size_t Extent>
            requires std::is_trivially_copyable_v<std::remove_cv_t<T>>
        DynamicVertexBufferHandle UploadDynamicVertexBuffer(std::span<T, Extent> data)
        {
            return UploadDynamicVertexBuffer(data.data(), data.size() * sizeof(T));
        }

        DynamicIndexBufferHandle UploadDynamicIndexBuffer(const void* data, size_t size);

        template <typename T, size_t Extent>
            requires std::is_trivially_copyable_v<std::remove_cv_t<T>>
        DynamicIndexBufferHandle UploadDynamicIndexBuffer(std::span<T, Extent> data)
        {
            return UploadDynamicIndexBuffer(data.data(), data.size() * sizeof(T));
        }

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
