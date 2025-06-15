#pragma once
#include "Array.h"
#include "Shader.h"

namespace TY
{
    struct GpgpuParams_impl
    {
        ComputeShader cs;
        int elementCount;
        int elementStride;
    };

    struct GpgpuParams
    {
        ComputeShader cs;
        int elementCount;
    };

    class Gpgpu_impl
    {
    public:
        Gpgpu_impl() = default;

        Gpgpu_impl(const GpgpuParams_impl& params);

        void compute(void* data);

        int elementCount() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    template <typename DataType>
    class Gpgpu : Gpgpu_impl
    {
    public:
        static constexpr int Stride = sizeof(DataType);

        Gpgpu() = default;

        Gpgpu(const GpgpuParams& params)
            : Gpgpu_impl{
                {
                    .cs = params.cs,
                    .elementCount = params.elementCount,
                    .elementStride = Stride
                }
            }
        {
            m_data.resize(params.elementCount);
        }

        Array<DataType>& data()
        {
            return m_data;
        }

        const Array<DataType>& data() const
        {
            return m_data;
        }

        void compute()
        {
            assert(m_data.size() == Gpgpu_impl::elementCount());
            Gpgpu_impl::compute(m_data.data());
        }

    private:
        Array<DataType> m_data{};
    };
}
