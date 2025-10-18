#pragma once
#include "ConstantBufferArray.h"

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
        operator ConstantBufferArrayImpl() const;

    private:
        ConstantBufferArrayImpl m_impl{Empty};
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
