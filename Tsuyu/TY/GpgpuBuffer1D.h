#pragma once
#include "Array.h"
#include "Intergral3D.h"
#include "Empty.h"

namespace TY
{
    namespace detail
    {
        struct IGpgpuBuffer
        {
            virtual ~IGpgpuBuffer() = default;

            virtual void* getDataPointer() = 0;

            virtual int getElementCount() const = 0;

            virtual int getElementStride() const = 0;

            virtual Point3D getSize3D() const = 0;
        };

        struct IReadonlyGpgpu : IGpgpuBuffer
        {
        };

        struct IWritableGpgpu : IReadonlyGpgpu
        {
        };

        template <typename DataType, typename InterfaceType>
        class GpgpuBuffer1D
        {
        public:
            static_assert(std::is_base_of<IGpgpuBuffer, InterfaceType>::value);

            static constexpr int ElementStride = sizeof(DataType);

            GpgpuBuffer1D(Empty_t)
            {
            }

            GpgpuBuffer1D(int elementCount = 0) : p_impl(std::make_shared<Impl>(elementCount))
            {
            }

            std::shared_ptr<IReadonlyGpgpu> asReadonly()
            {
                return p_impl;
            }

            operator std::shared_ptr<InterfaceType>()
            {
                return p_impl;
            }

            const Array<DataType>& data() const
            {
                return p_impl->m_data;
            }

            Array<DataType>& data()
            {
                return p_impl->m_data;
            }

            void resize(int newCount)
            {
                assert(p_impl);

                if (p_impl && newCount != p_impl->m_elementCount)
                {
                    *p_impl = Impl(newCount);
                }
            }

        private:
            struct Impl : InterfaceType
            {
                int m_elementCount{};
                Array<DataType> m_data{};

                Impl() = default;

                Impl(int count)
                    : m_elementCount(count),
                      m_data(count)

                {
                }

                void* getDataPointer() override { return m_data.data(); }

                int getElementCount() const override { return m_elementCount; }

                int getElementStride() const override { return ElementStride; }

                Point3D getSize3D() const override { return Point3D(m_elementCount, 1, 1); }
            };

            std::shared_ptr<Impl> p_impl{};

            GpgpuBuffer1D(const std::shared_ptr<Impl>& impl) : p_impl(impl)
            {
            }
        };

        template <typename DataType, typename InterfaceType>
        class GpgpuBuffer2D
        {
        public:
            static_assert(std::is_base_of<IGpgpuBuffer, InterfaceType>::value);

            static constexpr int ElementStride = sizeof(DataType);

            GpgpuBuffer2D(Empty_t)
            {
            }

            GpgpuBuffer2D(int x = 0, int y = 0) : p_impl(std::make_shared<Impl>(x, y))
            {
            }

            std::shared_ptr<IReadonlyGpgpu> asReadonly()
            {
                return p_impl;
            }

            operator std::shared_ptr<InterfaceType>()
            {
                return p_impl;
            }

            const Array<DataType>& data() const
            {
                return p_impl->m_data;
            }

            Array<DataType>& data()
            {
                return p_impl->m_data;
            }

            void resize(int newX, int newY)
            {
                assert(p_impl);

                if (p_impl && newX != p_impl->m_sizeX || newY != p_impl->m_sizeY)
                {
                    *p_impl = Impl(newX, newY);
                }
            }

            DataType& at(int x, int y)
            {
                return p_impl->m_data[p_impl->m_sizeX * y + x];
            }

            const DataType& at(int x, int y) const
            {
                return p_impl->m_data[p_impl->m_sizeX * y + x];
            }

        private:
            struct Impl : InterfaceType
            {
                int m_sizeX{};
                int m_sizeY{};
                int m_elementCount{};
                Array<DataType> m_data{};

                Impl() = default;

                Impl(int x, int y)
                    : m_sizeX(x),
                      m_sizeY(y),
                      m_elementCount(x * y),
                      m_data(m_elementCount)
                {
                }

                void* getDataPointer() override { return m_data.data(); }

                int getElementCount() const override { return m_elementCount; }

                int getElementStride() const override { return ElementStride; }

                Point3D getSize3D() const override { return Point3D(m_sizeX, m_sizeY, 1); }
            };

            std::shared_ptr<Impl> p_impl{};

            GpgpuBuffer2D(const std::shared_ptr<Impl>& impl) : p_impl(impl)
            {
            }
        };
    }

    template <typename DataType>
    using WritableGpgpuBuffer1D = detail::GpgpuBuffer1D<DataType, detail::IWritableGpgpu>;

    template <typename DataType>
    using ReadonlyGpgpuBuffer1D = detail::GpgpuBuffer1D<DataType, detail::IReadonlyGpgpu>;

    template <typename DataType>
    using WritableGpgpuBuffer2D = detail::GpgpuBuffer2D<DataType, detail::IWritableGpgpu>;

    template <typename DataType>
    using ReadonlyGpgpuBuffer2D = detail::GpgpuBuffer2D<DataType, detail::IReadonlyGpgpu>;
}
