#include "pch.h"
#include "ComputePipelineState.h"

#include "EngineHotReloader.h"
#include "EngineRenderContext.h"
#include "RootSignature.h"
#include "TY/Logger.h"
#include "TY/System.h"

using namespace TY;
using namespace TY::detail;

struct ComputePipelineState::Impl : IEngineHotReloadable
{
    ComputePipelineStateParams m_params;

    uint64_t m_timestamp{};
    bool m_valid{};

    ComPtr<ID3D12PipelineState> m_pipelineState;
    RootSignature m_rootSignature;

    Impl(const ComputePipelineStateParams& params)
        : m_params(params)
    {
        Impl::HotReload();
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }

    void HotReload() override
    {
        m_timestamp = System::FrameCount();
        m_valid = false;

        if (m_params.computeShader.isEmpty())
        {
            return;
        }

        m_rootSignature = RootSignature{{m_params.descriptorTable}};

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.getPointer();
        desc.CS.pShaderBytecode = m_params.computeShader.getBlob()->GetBufferPointer();
        desc.CS.BytecodeLength = m_params.computeShader.getBlob()->GetBufferSize();

        const auto device = EngineRenderContext::GetDevice();
        if (const auto hr = device->CreateComputePipelineState(
                &desc, IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError.writeln(std::format("Failed to create compute pipeline state: {}", hr));
            return;
        }

        // LogInfo.writeln("ComputePipelineState created successfully.");

        m_valid = true;
    }

    void CommandSet() const
    {
        if (not m_valid) return;

        const auto commandList = EngineRenderContext::ActiveCommandList();
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetComputeRootSignature(m_rootSignature.getPointer());
    }
};

namespace TY::detail
{
    ComputePipelineState::ComputePipelineState(const ComputePipelineStateParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
#ifdef _DEBUG
        EngineHotReloader::TrackAsset(
            p_impl, {p_impl->m_params.computeShader.timestamp()});
#endif
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
