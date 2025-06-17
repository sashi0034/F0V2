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
    };

    struct IWritableGpgpu : IGpgpuBuffer
    {
    };

    struct IReadonlyGpgpu : IGpgpuBuffer
    {
    };

    template <typename DataType, typename InterfaceType>
    class GpgpuBuffer
    {
    public:
        static_assert(std::is_base_of<IGpgpuBuffer, InterfaceType>::value);

        static constexpr int ElementStride = sizeof(DataType);

        GpgpuBuffer()
        {
        }

        GpgpuBuffer(int elementCount) : p_impl(std::make_shared<Impl>(elementCount))
        {
        }

        operator std::shared_ptr<InterfaceType>()
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

    private:
        struct Impl : InterfaceType
        {
            Array<DataType> m_data{};
            int m_elementCount{};

            Impl() = default;

            Impl(int count)
                : m_data(count),
                  m_elementCount(count)
            {
            }

            void* getDataPointer() override { return m_data.data(); }

            int getElementCount() const override { return m_elementCount; }

            int getElementStride() const override { return ElementStride; }
        };

        std::shared_ptr<Impl> p_impl{};

        GpgpuBuffer(const std::shared_ptr<Impl>& impl) : p_impl(impl)
        {
        }
    };

    template <typename DataType>
    using WritableGpgpuBuffer = GpgpuBuffer<DataType, IWritableGpgpu>;

    template <typename DataType>
    using ReadonlyGpgpuBuffer = GpgpuBuffer<DataType, IReadonlyGpgpu>;
}
