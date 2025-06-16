#pragma once
#include "Array.h"
#include "ConstantBufferUploader.h"
#include "Shader.h"

namespace TY
{
    struct GpgpuParams
    {
        ComputeShader cs;
        ConstantBufferUploader_impl cb1{Empty};
        int elementCount;
    };

    struct GpgpuParams_detail : GpgpuParams
    {
        int elementStride;

        GpgpuParams_detail() = default;

        GpgpuParams_detail(const GpgpuParams& params, int elementStride_)
            : GpgpuParams{params},
              elementStride{elementStride_}
        {
        }
    };

    class Gpgpu_impl
    {
    public:
        Gpgpu_impl() = default;

        Gpgpu_impl(const GpgpuParams_detail& params);

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
            : Gpgpu_impl{{params, Stride}}
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
