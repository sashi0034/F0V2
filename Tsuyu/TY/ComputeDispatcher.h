#pragma once
#include "Array.h"
#include "CbvSrvUav.h"
#include "ConstantBuffer.h"
#include "GraphicsOptions.h"
#include "MaterialList.h"
#include "Shader.h"

namespace TY
{
    struct ComputeDispatcherParams
    {
        ComputeShader cs{};

        Array<GraphicsSamplerOptions> samplers{GraphicsSamplerOptions()};

        DescriptorList<ConstantBufferImpl> cbv{}; // from b0

        DescriptorList<ShaderResourceType> srv{}; // from t0

        DescriptorList<UnorderedAccessType> uav{}; // from u0

        int dynamicCbvCount{};

        ComputeDispatcherParams& setCS(const ComputeShader& cs_);

        ComputeDispatcherParams& setSamplers(const Array<GraphicsSamplerOptions>& samplers_);

        ComputeDispatcherParams& setCbv(const DescriptorList<ConstantBufferImpl>& cbv_);

        ComputeDispatcherParams& setSrv(const DescriptorList<ShaderResourceType>& srv_);

        ComputeDispatcherParams& setUav(const DescriptorList<UnorderedAccessType>& uav_);

        ComputeDispatcherParams& setDynamicCbvCount(int count);
    };

    class ComputeDispatcher
    {
    public:
        ComputeDispatcher() = default;

        ComputeDispatcher(const ComputeDispatcherParams& params);

        void dispatch(int threadGroupCountX, int threadGroupCountY = 1, int threadGroupCountZ = 1) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
