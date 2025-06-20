#pragma once

#include "Array.h"
#include "ConstantBufferUploader.h"
#include "GpgpuBuffer1D.h"
#include "Shader.h"

namespace TY
{
    struct GpgpuParams
    {
        ComputeShader cs{};
        Array<std::shared_ptr<detail::IReadonlyGpgpu>> readonlyBuffer{};
        Array<std::shared_ptr<detail::IWritableGpgpu>> writableBuffer{};
        ConstantBufferUploader_impl cb2{Empty};

        GpgpuParams& setCS(const ComputeShader& cs_)
        {
            cs = cs_;
            return *this;
        }

        GpgpuParams& setReadonlyBuffer(const Array<std::shared_ptr<detail::IReadonlyGpgpu>>& buffers_)
        {
            readonlyBuffer = buffers_;
            return *this;
        }

        GpgpuParams& setWritableBuffer(const Array<std::shared_ptr<detail::IWritableGpgpu>>& buffers_)
        {
            writableBuffer = buffers_;
            return *this;
        }

        GpgpuParams& setCB2(const ConstantBufferUploader_impl& cb)
        {
            cb2 = cb;
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
