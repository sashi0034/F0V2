#pragma once
#include "ConstantBufferArray.h"

namespace TY
{
    template <class T>
    class ConstantBufferWrapper
    {
    public:
        void upload()
        {
            m_uploader.upload(m_value);
        }

        void uploadValue(const T& value)
        {
            m_value = value;
            m_uploader.upload(m_value);
        }

        const T& value() const
        {
            return m_value;
        }

        T* operator ->()
        {
            return &m_value;
        }

        const T* operator ->() const
        {
            return &m_value;
        }

        operator ConstantBufferArray<T>&()
        {
            return m_uploader;
        }

        operator const ConstantBufferArray<T>&() const
        {
            return m_uploader;
        }

    private:
        ConstantBufferArray<T> m_uploader{1};
        T m_value{};
    };
}
