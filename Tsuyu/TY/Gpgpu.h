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
        Array<std::shared_ptr<detail::IReadonlyGpgpu>> readonlyBuffer{};
        Array<std::shared_ptr<detail::IWritableGpgpu>> writableBuffer{};
        Array<ConstantBufferUploaderCore> cbv10AndLater{Empty};

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

        GpgpuParams& setCbv10AndLater(const Array<ConstantBufferUploaderCore>& cb)
        {
            cbv10AndLater = cb;
            return *this;
        }
    };

    class Gpgpu
    {
    public:
        Gpgpu() = default;

        Gpgpu(const GpgpuParams& params);

        void compute();

        static void SequenceCompute(const Array<Gpgpu>& list);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
