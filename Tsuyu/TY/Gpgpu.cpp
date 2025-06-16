#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBuffer.h"
#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    const DescriptorTable descriptorTable = {{2, 0, 1}};

    struct BufferInfo_b0
    {
        uint32_t elementCount{};
    };
}

struct Gpgpu_impl::Impl
{
    GpgpuParams_detail m_params{};

    ConstantBuffer<BufferInfo_b0> m_cb0{};

    UnorderedAccessTransfer m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

    Impl(const GpgpuParams_detail& params) : m_params(params)
    {
        m_ua = UnorderedAccessTransfer({
            .elementCount = m_params.elementCount,
            .elementStride = m_params.elementStride
        });

        m_computePipelineState = ComputePipelineState({
            .computeShader = params.cs,
            .descriptorTable = descriptorTable
        });

        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_computePipelineState.descriptorTable(),
            .materialCounts = {1},
            .descriptors = {CbSrUaSet{{m_cb0, params.cb1}, {}, {{m_ua}}}}
        });

        m_cb0->elementCount = static_cast<uint32_t>(m_params.elementCount);
        m_cb0.upload();
    }

    void Compute(void* data)
    {
        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);
        m_ua.upload(data);

        m_computePipelineState.commandSet();
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Compute, 0);

        const auto commandList = EngineRenderContext::ActiveCommandList();
        constexpr double groutCountX = 64.0;
        commandList->Dispatch(static_cast<UINT>(ceil(m_params.elementCount / groutCountX)), 1, 1);

        m_ua.readback(data);
    }
};

namespace TY
{
    Gpgpu_impl::Gpgpu_impl(const GpgpuParams_detail& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    void Gpgpu_impl::compute(void* data)
    {
        if (p_impl) p_impl->Compute(data);
    }

    int Gpgpu_impl::elementCount() const
    {
        return p_impl ? p_impl->m_params.elementCount : 0;
    }
}
