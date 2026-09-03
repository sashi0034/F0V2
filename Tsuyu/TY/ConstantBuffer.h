#pragma once
#include "Empty.h"

namespace TY
{
    class ConstantBufferImpl
    {
    public:
        [[nodiscard]]
        ConstantBufferImpl(Empty_t)
        {
        }

        [[nodiscard]]
        ConstantBufferImpl(uint32_t sizeInBytes);

        void upload(const void* data) const;

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        size_t alignedSize() const;

        [[nodiscard]]
        uint64_t bufferLocation() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    template <class T>
    class ConstantBuffer : public ConstantBufferImpl
    {
    public:
        [[nodiscard]]
        ConstantBuffer(Empty_t) : ConstantBufferImpl(Empty)
        {
        }

        [[nodiscard]]
        ConstantBuffer() : ConstantBufferImpl(sizeof(T))
        {
        }

        void upload(const T& data) const
        {
            ConstantBufferImpl::upload(&data);
        }
    };
}
