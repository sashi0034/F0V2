#pragma once
#include "Array.h"

namespace TY
{
    struct IGpgpuBuffer
    {
        virtual ~IGpgpuBuffer() = default;

        virtual void* getDataPointer() = 0;

        virtual int getElementCount() const = 0;

        virtual int getElementStride() const = 0;

        virtual bool getReadonly() const = 0;
    };

    template <typename DataType>
    class GpgpuBuffer
    {
    public:
        static constexpr int ElementStride = sizeof(DataType);

        GpgpuBuffer()
        {
        }

        operator std::shared_ptr<IGpgpuBuffer>()
        {
            return p_impl;
        }

        const Array<DataType>& data() const
        {
            return p_impl->m_data;
        }

        // TODO: dirty flag 実装
        Array<DataType>& data()
        {
            return p_impl->m_data;
        }

        static GpgpuBuffer Writable(int elementCount)
        {
            return std::make_shared<Impl>(elementCount, false);
        }

        static GpgpuBuffer Readonly(int elementCount)
        {
            return std::make_shared<Impl>(elementCount, true);
        }

    private:
        struct Impl : IGpgpuBuffer
        {
            Array<DataType> m_data{};
            int m_elementCount{};
            bool m_readonly{};

            Impl() = default;

            Impl(int count, bool isReadonly)
                : m_data(count),
                  m_elementCount(count),
                  m_readonly(isReadonly)
            {
            }

            void* getDataPointer() override { return m_data.data(); }

            int getElementCount() const override { return m_elementCount; }

            int getElementStride() const override { return ElementStride; }

            bool getReadonly() const override { return m_readonly; }
        };

        std::shared_ptr<Impl> p_impl{};

        GpgpuBuffer(const std::shared_ptr<Impl>& impl) : p_impl(impl)
        {
        }
    };
}
