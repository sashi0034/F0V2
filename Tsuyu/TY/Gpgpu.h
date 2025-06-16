#pragma once

#include "Array.h"
#include "ConstantBufferUploader.h"
#include "GpgpuBuffer.h"
#include "Shader.h"

namespace TY
{
    struct GpgpuParams
    {
        ComputeShader cs{};
        Array<std::shared_ptr<IGpgpuBuffer>> buffers{};
        ConstantBufferUploader_impl cb1{Empty};

        GpgpuParams& setCS(const ComputeShader& cs_)
        {
            cs = cs_;
            return *this;
        }

        GpgpuParams& setBuffers(const Array<std::shared_ptr<IGpgpuBuffer>>& buffers_)
        {
            buffers = buffers_;
            return *this;
        }

        GpgpuParams& setCB1(const ConstantBufferUploader_impl& cb1_)
        {
            cb1 = cb1_;
            return *this;
        }
    };

    class Gpgpu
    {
    public:
        Gpgpu() = default;

        Gpgpu(const GpgpuParams& params);

        void compute();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
