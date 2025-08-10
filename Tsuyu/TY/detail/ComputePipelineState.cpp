#include "pch.h"
#include "ComputePipelineState.h"

#include "EngineHotReloader.h"
#include "EnginePresetAsset.h"
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

    ComPtr<ID3D12PipelineState> m_pso;
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

        m_rootSignature = RootSignature(RootSignatureParams{
            .samplers = {
                GraphicsSamplerOptions{}
            },
            .descriptorTable = m_params.descriptorTable,
            .explicitRegisterStarts = m_params.explicitRegisterStarts
        });

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.getPointer();

        const auto cs = m_params.computeShader.isEmpty() ? EnginePresetAsset::GetStubCS() : m_params.computeShader;
        desc.CS.pShaderBytecode = cs.getBlob()->GetBufferPointer();
        desc.CS.BytecodeLength = cs.getBlob()->GetBufferSize();

        const auto device = EngineRenderContext::GetDevice();
        if (const auto hr = device->CreateComputePipelineState(
                &desc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError.writeln(std::format("Failed to create compute pipeline state: {}", hr));
            return;
        }

        // LogInfo.writeln("ComputePipelineState created successfully.");
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::ActiveCommandList();
        commandList->SetPipelineState(m_pso.Get());
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
