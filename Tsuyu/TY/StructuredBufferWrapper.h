#pragma once
#include "Array.h"
#include "StructuredBuffer.h"

namespace TY
{
    template <typename T>
    class StructuredBufferWrapper
    {
    public:
        StructuredBufferWrapper() = default;

        StructuredBufferWrapper(int count) : m_buffer(count), m_data(count)
        {
        }

        void upload()
        {
            m_buffer.upload(m_data);
        }

        [[nodiscard]]
        int count() const
        {
            return m_data.size();
        }

        [[nodiscard]]
        const T& operator[](int i) const
        {
            return m_data[i];
        }

        [[nodiscard]]
        T& operator[](int i)
        {
            return m_data[i];
        }

        [[nodiscard]]
        operator StructuredBuffer() const
        {
            return m_buffer;
        }

    private:
        StructuredBufferT<T> m_buffer{};
        Array<T> m_data{};
    };
}
