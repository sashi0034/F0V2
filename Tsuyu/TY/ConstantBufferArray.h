#pragma once
#include <span>

#include "Array.h"
#include "Empty.h"

namespace TY
{
    class ConstantBufferArrayImpl
    {
    public:
        [[nodiscard]]
        ConstantBufferArrayImpl(Empty_t)
        {
        }

        [[nodiscard]]
        ConstantBufferArrayImpl(uint32_t sizeInBytes, uint32_t materialCount);

        [[nodiscard]]
        bool isEmpty() const;

        void upload(const void* data, uint32_t materialCount) const;

        [[nodiscard]]
        uint32_t materialCount() const;

        [[nodiscard]]
        size_t sizeInBytes() const;

        [[nodiscard]]
        size_t alignedSize() const;

        [[nodiscard]]
        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <typename T>
    class ConstantBufferArray : public ConstantBufferArrayImpl
    {
    public:
        static constexpr uint32_t sizeInBytes = sizeof(T);

        [[nodiscard]]
        ConstantBufferArray(Empty_t) : ConstantBufferArrayImpl(Empty)
        {
        }

        [[nodiscard]]
        ConstantBufferArray(int materialCount) : ConstantBufferArrayImpl(sizeInBytes, materialCount)
        {
        }

        [[nodiscard]]
        ConstantBufferArray(const T& data) : ConstantBufferArray(data.size())
        {
            upload(data);
        }

        [[nodiscard]]
        ConstantBufferArray(const Array<T>& data) : ConstantBufferArray(data.size())
        {
            upload(data);
        }

        void upload(const T& data) const
        {
            ConstantBufferArrayImpl::upload(&data, 1);
        }

        void upload(const Array<T>& data) const
        {
            ConstantBufferArrayImpl::upload(data.data(), data.size());
        }

        void upload(std::span<const T> data) const
        {
            ConstantBufferArrayImpl::upload(data.data(), static_cast<uint32_t>(data.size()));
        }
    };
}
