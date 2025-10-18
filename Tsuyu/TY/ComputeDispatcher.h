#pragma once
#include "Array.h"
#include "CbvSrvUav.h"
#include "ConstantBufferArray.h"
#include "Shader.h"

namespace TY
{
    struct ComputeDispatcherParams
    {
        ComputeShader cs{};

        Array<ConstantBufferArrayImpl> cbv{}; // from b0

        Array<ShaderResourceType> srv{}; // from t0

        Array<UnorderedAccessType> uav{}; // from u0

        ComputeDispatcherParams& setCS(const ComputeShader& cs_);

        ComputeDispatcherParams& setCbv(const Array<ConstantBufferArrayImpl>& cbv_);

        ComputeDispatcherParams& setSrv(const Array<ShaderResourceType>& srv_);

        ComputeDispatcherParams& setUav(const Array<UnorderedAccessType>& uav_);
    };

    class ComputeDispatcher
    {
    public:
        ComputeDispatcher() = default;

        ComputeDispatcher(const ComputeDispatcherParams& params);

        void dispatchToDraw(int threadGroupCountX, int threadGroupCountY = 1, int threadGroupCountZ = 1) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
