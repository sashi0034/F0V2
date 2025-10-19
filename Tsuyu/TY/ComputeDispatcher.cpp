#include "pch.h"
#include "ComputeDispatcher.h"

#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct ComputeDispatcher::Impl
{
    ComputePipelineState m_pso{};

    DescriptorHeap m_descriptorHeap{};

    Array<UnorderedAccessType> m_uavList{};

    Impl(const ComputeDispatcherParams& params)
    {
        m_uavList = params.uav;

        auto descriptorHeap = DescriptorHeapParams{
            .table = {
                {params.cbv.size(), params.srv.size(), params.uav.size()}
            },
            .materialCounts = {1},
            .descriptors = {
                CbvSrvUavSet{params.cbv, {params.srv}, {params.uav}}
            }
        };

        m_pso = ComputePipelineState{
            ComputePipelineStateParams{
                .computeShader = params.cs,
                .descriptorTable = descriptorHeap.table
            }
        };

        m_descriptorHeap = DescriptorHeap{descriptorHeap};
    }

    void Dispatch(int threadGroupCountX, int threadGroupCountY, int threadGroupCountZ) const
    {
        for (auto& uav : m_uavList)
        {
            if (uav.isHolds<UnorderedTextureHandle>())
            {
                uav.get<UnorderedTextureHandle>().transitionResourceState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            }
            else
            {
                assert(false); // TODO
            }
        }

        m_pso.commandSet(CommandListType::Draw);

        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetComputeTable(0);

        EngineRenderContext::TargetCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);

        for (auto& uav : m_uavList)
        {
            if (uav.isHolds<UnorderedTextureHandle>())
            {
                uav.get<UnorderedTextureHandle>().transitionResourceState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            }
            else
            {
                assert(false); // TODO
            }
        }
    }
};

namespace TY
{
    ComputeDispatcherParams& ComputeDispatcherParams::setCS(const ComputeShader& cs_)
    {
        cs = cs_;
        return *this;
    }

    ComputeDispatcherParams& ComputeDispatcherParams::setCbv(const Array<ConstantBufferArrayImpl>& cbv_)
    {
        cbv = cbv_;
        return *this;
    }

    ComputeDispatcherParams& ComputeDispatcherParams::setSrv(const Array<ShaderResourceType>& srv_)
    {
        srv = srv_;
        return *this;
    }

    ComputeDispatcherParams& ComputeDispatcherParams::setUav(const Array<UnorderedAccessType>& uav_)
    {
        uav = uav_;
        return *this;
    }

    ComputeDispatcher::ComputeDispatcher(const ComputeDispatcherParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    void ComputeDispatcher::dispatch(int threadGroupCountX, int threadGroupCountY, int threadGroupCountZ) const
    {
        if (p_impl) p_impl->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
    }
}
