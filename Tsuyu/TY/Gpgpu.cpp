#include "pch.h"
#include "Gpgpu.h"

#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    const DescriptorTable descriptorTable = {{0, 0, 1}};
}

struct Gpgpu_impl::Impl
{
    GpgpuParams_impl m_params{};

    UnorderedAccessUploader m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

    Impl(const GpgpuParams_impl& params) : m_params(params)
    {
        m_ua = UnorderedAccessUploader({
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
            .descriptors = {CbSrUaSet{{}, {}, {{m_ua}}}}
        });
    }

    void Compute(const void* data)
    {
        m_ua.upload(data);

        m_computePipelineState.commandSet();
        m_descriptorHeap.CommandSet();

        const auto commandList = EngineRenderContext::GetCommandList();
        // commandList->Dispatch((UINT)ceil(m_params.elementCount / 64.0), 1, 1); // TODO
        commandList->Dispatch(1, 1, 1); // TODO
    }
};

namespace TY
{
    Gpgpu_impl::Gpgpu_impl(const GpgpuParams_impl& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    void Gpgpu_impl::compute(const void* data)
    {
        if (p_impl) p_impl->Compute(data);
    }

    int Gpgpu_impl::elementCount() const
    {
        return p_impl ? p_impl->m_params.elementCount : 0;
    }
}
