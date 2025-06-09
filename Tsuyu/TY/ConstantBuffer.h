#pragma once
#include "ConstantBufferUploader.h"

namespace TY
{
    template <class T>
    class ConstantBuffer
    {
    public:
        void upload()
        {
            m_uploader.upload(m_value);
        }

        T* operator ->()
        {
            return &m_value;
        }

        const T* operator ->() const
        {
            return &m_value;
        }

        operator ConstantBufferUploader<T>&()
        {
            return m_uploader;
        }

        operator const ConstantBufferUploader<T>&() const
        {
            return m_uploader;
        }

    private:
        ConstantBufferUploader<T> m_uploader{1};
        T m_value{};
    };
}
