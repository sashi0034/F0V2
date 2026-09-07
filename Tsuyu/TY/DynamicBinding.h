#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

#include "Array.h"
#include "DynamicHandle.h"
#include "detail/DynamicDescriptorEntry.h"

namespace TY
{
    namespace DynamicBinding
    {
        DynamicVertexBufferHandle UploadDynamicVertexBuffer(
            const void* data, size_t sizeInBytes, size_t strideInBytes);

        template <typename T, size_t Extent>
            requires std::is_trivially_copyable_v<std::remove_cv_t<T>>
        DynamicVertexBufferHandle UploadDynamicVertexBuffer(std::span<T, Extent> data)
        {
            return UploadDynamicVertexBuffer(data.data(), data.size() * sizeof(T), sizeof(T));
        }

        DynamicIndexBufferHandle UploadDynamicIndexBuffer(const void* data, size_t sizeInBytes);

        template <typename T, size_t Extent>
            requires std::is_same_v<std::remove_cv_t<T>, uint16_t>
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

        void SetDynamicCbv(int slotIndex, const void* data, size_t size);

        template <typename T>
        void SetDynamicCbv(int slotIndex, const T& data)
        {
            return SetDynamicCbv(slotIndex, &data, sizeof(T));
        }

        void SetDynamicCbv(int slotIndex, DynamicCbvHandle cbv);

        DynamicSrvHandle UploadDynamicStructuredBuffer(const void* data, size_t sizeInBytes);

        template <typename T, size_t Extent>
            requires std::is_trivially_copyable_v<std::remove_cv_t<T>>
        DynamicSrvHandle UploadDynamicStructuredBuffer(std::span<T, Extent> data)
        {
            return UploadDynamicStructuredBuffer(data.data(), data.size_bytes());
        }

        void SetDynamicSrv(int slotIndex, DynamicSrvHandle srv);

        void FlushAsGraphics(
            int rootParameterOffset,
            const Array<detail::DynamicDescriptorEntry>& dynamicDescriptorTables);

        void FlushAsCompute(
            int rootParameterOffset,
            const Array<detail::DynamicDescriptorEntry>& dynamicDescriptorTables);
    }
}
