#include "pch.h"
#include "ComputePipelineState.h"

#include "EngineRenderContext.h"
#include "RootSignature.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

struct ComputePipelineState::Impl
{
    ComPtr<ID3D12PipelineState> m_pipelineState;
    RootSignature m_rootSignature;
    ComputePipelineStateParams m_params;

    Impl(const ComputePipelineStateParams& params)
        : m_params(params)
    {
        m_rootSignature = RootSignature{{params.descriptorTable}};

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.getPointer();
        desc.CS.pShaderBytecode = params.computeShader.getBlob()->GetBufferPointer();
        desc.CS.BytecodeLength = params.computeShader.getBlob()->GetBufferSize();

        const auto device = EngineRenderContext::GetDevice();
        if (const auto hr = device->CreateComputePipelineState(
                &desc, IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError.writeln(std::format("Failed to create compute pipeline state: {}", hr));
            return;
        }

        LogInfo.writeln("ComputePipelineState created successfully.");
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::GetCommandList();
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetComputeRootSignature(m_rootSignature.getPointer());
    }
};

namespace TY::detail
{
    ComputePipelineState::ComputePipelineState(const ComputePipelineStateParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    DescriptorTable ComputePipelineState::descriptorTable() const
    {
        return p_impl ? p_impl->m_params.descriptorTable : DescriptorTable{};
    }

    void ComputePipelineState::commandSet() const
    {
        if (p_impl) p_impl->CommandSet();
    }
}
