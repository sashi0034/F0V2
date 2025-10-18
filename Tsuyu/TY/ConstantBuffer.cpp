#include "pch.h"
#include "ConstantBuffer.h"

namespace TY
{
    ConstantBufferImpl::ConstantBufferImpl(uint32_t sizeInBytes)
        : m_impl(sizeInBytes, 1)
    {
    }

    void ConstantBufferImpl::upload(const void* data) const
    {
        m_impl.upload(data, 1);
    }

    ConstantBufferImpl::operator ConstantBufferArrayImpl() const
    {
        return m_impl;
    }
}
