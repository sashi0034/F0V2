#pragma once
#include "Array.h"

namespace TY
{
    class ConstantBuffer_impl
    {
    public:
        ConstantBuffer_impl() = default;

        ConstantBuffer_impl(uint32_t sizeInBytes, uint32_t count);

        void upload(const void* data, uint32_t count) const;

        uint32_t count() const;

        size_t alignedSize() const;

        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <typename T>
    class ConstantBuffer : public ConstantBuffer_impl
    {
    public:
        static constexpr uint32_t sizeInBytes = sizeof(T);

        ConstantBuffer() : ConstantBuffer_impl()
        {
        }

        ConstantBuffer(int count) : ConstantBuffer_impl(sizeInBytes, count)
        {
        }

        ConstantBuffer(const T& data) : ConstantBuffer(data.size())
        {
            upload(data);
        }

        ConstantBuffer(const Array<T>& data) : ConstantBuffer(data.size())
        {
            upload(data);
        }

        void upload(const T& data, int index = 0) const
        {
            ConstantBuffer_impl::upload(&data, 1);
        }

        void upload(const Array<T>& data) const
        {
            ConstantBuffer_impl::upload(data.data(), data.size());
        }
    };
}
