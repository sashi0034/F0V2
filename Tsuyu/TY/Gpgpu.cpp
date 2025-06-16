#include "pch.h"
#include "Gpgpu.h"

#include "ConstantBuffer.h"
#include "Logger.h"
#include "detail/ComputePipelineState.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    constexpr int maxUavCount = 8;

    const DescriptorTable descriptorTable = {{2, 0, maxUavCount}};

    struct BufferInfo_b0
    {
        std::array<uint32_t, maxUavCount> elementCount{};
    };
}

struct Gpgpu::Impl
{
    GpgpuParams m_params{};

    ConstantBuffer<BufferInfo_b0> m_cb0{};

    Array<UnorderedAccessTransfer> m_ua{};

    ComputePipelineState m_computePipelineState{};

    DescriptorHeap m_descriptorHeap{};

    Impl(const GpgpuParams& params) : m_params(params)
    {
        if (params.buffers.size() > maxUavCount)
        {
            LogError.writeln("Gpgpu: Too many buffers specified. Maximum is " + std::to_string(maxUavCount));
            return;
        }

        m_ua.resize(maxUavCount);
        for (int i = 0; i < params.buffers.size(); ++i)
        {
            m_ua[i] = UnorderedAccessTransfer({
                .elementCount = params.buffers[i]->getElementCount(),
                .elementStride = params.buffers[i]->getElementStride()
            });
        }

        m_computePipelineState = ComputePipelineState({
            .computeShader = params.cs,
            .descriptorTable = descriptorTable
        });

        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_computePipelineState.descriptorTable(),
            .materialCounts = {1},
            .descriptors = {CbSrUaSet{{m_cb0, params.cb1}, {}, m_ua.toColumnVector()}}
        });

        for (int i = 0; i < params.buffers.size(); ++i)
        {
            m_cb0->elementCount[i] = static_cast<uint32_t>(m_params.buffers[i]->getElementCount());
        }

        m_cb0.upload();
    }

    void Compute()
    {
        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);
        for (int i = 0; i < m_params.buffers.size(); ++i)
        {
            m_ua[i].upload(m_params.buffers[i]->getDataPointer());
        }

        m_computePipelineState.commandSet();
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Compute, 0);

        const auto commandList = EngineRenderContext::ActiveCommandList();
        constexpr double groutCountX = 64.0;
        const auto mainUA = m_ua[0];
        commandList->Dispatch(static_cast<UINT>(ceil(mainUA.elementCount() / groutCountX)), 1, 1);

        for (int i = 0; i < m_params.buffers.size(); ++i)
        {
            if (m_params.buffers[i]->getReadonly()) continue;
            m_ua[i].readback(m_params.buffers[i]->getDataPointer());
        }
    }
};

namespace TY
{
    Gpgpu::Gpgpu(const GpgpuParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    void Gpgpu::compute()
    {
        if (p_impl) p_impl->Compute();
    }
}
