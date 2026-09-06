#include "pch.h"
#include "ComputePipelineState.h"

#include "EngineHotReloader.h"
#include "EnginePresetAsset.h"
#include "RenderContext_singleton.h"
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

    ~Impl()
    {
        DisposeRenderObject();
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }

    void DisposeRenderObject()
    {
        RenderContext_singleton::SafeDisposeRenderObject(m_pso);
        RenderContext_singleton::SafeDisposeRenderObject(m_rootSignature.get());
    }

    void HotReload() override
    {
        m_timestamp = System::FrameCount();

        DisposeRenderObject();

        if (not m_params.computeShader.isEmpty())
        {
            if (createPipelineState(m_params))
            {
                return;
            }
        }

        LogWarning(L"ComputePipelineState: Failed to create PSO with user shaders, using stub shaders instead");

        const auto stubParams = makeStubParams(m_params);
        if (SUCCEEDED(createPipelineState(stubParams)))
        {
            return;
        }

        throw std::runtime_error("GraphicsPipelineState: Failed to create PSO.");
    }

    static ComputePipelineStateParams makeStubParams(const ComputePipelineStateParams& params)
    {
        auto stub = params;

        stub.computeShader = EnginePresetAsset::GetStubCS();

        return stub;
    }

    void CommandSet() const
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();
        commandList->SetPipelineState(m_pso.Get());
        commandList->SetComputeRootSignature(m_rootSignature.getPointer());
    }

private:
    bool createPipelineState(const ComputePipelineStateParams& params)
    {
        m_rootSignature = RootSignature(RootSignatureParams{
            .samplers = params.samplers,
            .descriptorTable = params.descriptorTable,
            .dynamicDescriptorTable = params.dynamicDescriptorTable,
        });

        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
        desc.pRootSignature = m_rootSignature.getPointer();

        const auto cs = params.computeShader;
        desc.CS.pShaderBytecode = cs.getBlob()->GetBufferPointer();
        desc.CS.BytecodeLength = cs.getBlob()->GetBufferSize();

        const auto device = RenderContext_singleton::GetDevice();
        if (const auto hr = device->CreateComputePipelineState(
                &desc, IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError.writeln(std::format("Failed to create compute pipeline state: {}", hr));
            return false;
        }

        return true;
    }
};

namespace TY::detail
{
    ComputePipelineState::ComputePipelineState(const ComputePipelineStateParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
#if defined(_DEBUG)
        EngineHotReloader::TrackAsset(
            p_impl, {p_impl->m_params.computeShader.timestamp()});
#endif
    }

    DescriptorTable ComputePipelineState::descriptorTable() const
    {
        return p_impl ? p_impl->m_params.descriptorTable : DescriptorTable{};
    }

    int ComputePipelineState::dynamicBindingRootParameterOffset() const
    {
        return p_impl ? p_impl->m_rootSignature.dynamicBindingRootParameterOffset() : 0;
    }

    const Array<DynamicDescriptorEntry>& ComputePipelineState::resolvedDynamicDescriptorTable() const
    {
        static const Array<DynamicDescriptorEntry> Empty{};
        return p_impl ? p_impl->m_rootSignature.resolvedDynamicDescriptorTable() : Empty;
    }

    void ComputePipelineState::commandSet(CommandListType commandList) const
    {
        if (p_impl) p_impl->CommandSet();
    }
}
