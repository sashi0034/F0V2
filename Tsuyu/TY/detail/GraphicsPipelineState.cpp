#include "pch.h"
#include "GraphicsPipelineState.h"

#include "TY/AssertObject.h"
#include "EngineHotReloader.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "IEngineHotReloadable.h"
#include "RootSignature.h"
#include "TY/Logger.h"
#include "TY/System.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> buildVertexInputLayout(const std::vector<VertexInputElement>& inputLayout)
    {
        std::vector<D3D12_INPUT_ELEMENT_DESC> result{};
        result.reserve(inputLayout.size());

        for (const auto& element : inputLayout)
        {
            D3D12_INPUT_ELEMENT_DESC desc = {};
            desc.SemanticName = element.semanticName.c_str();
            desc.SemanticIndex = element.semanticIndex;
            desc.Format = element.format;
            desc.InputSlot = 0;
            desc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            desc.InstanceDataStepRate = 0;
            result.push_back(desc);
        }

        return result;
    }
}

struct GraphicsPipelineState::Impl : IEngineHotReloadable
{
    uint64_t m_timestamp{};

    ComPtr<ID3D12PipelineState> m_pipelineState;
    RootSignature m_rootSignature;
    PipelineStateParams m_params;

    Impl(const PipelineStateParams& params) :
        m_params(params)
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

        if (SUCCEEDED(createPipelineState(m_params))) return;

        LogWarning.writeln(L"failed to create pipeline state with user shaders, using stub shaders instead");

        auto params2 = m_params;
        params2.pixelShader = EnginePresetAsset::GetStubPS();
        params2.vertexShader = EnginePresetAsset::GetStubVS();

        if (SUCCEEDED(createPipelineState(params2))) return;

        throw std::runtime_error("failed to create pipeline state");
    }

    HRESULT createPipelineState(const PipelineStateParams& params)
    {
        const auto device = EngineRenderContext::GetDevice();
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};

        const auto vs = params.vertexShader.isEmpty() ? EnginePresetAsset::GetStubVS() : params.vertexShader;
        pipelineDesc.VS.pShaderBytecode = vs.getBlob()->GetBufferPointer();
        pipelineDesc.VS.BytecodeLength = vs.getBlob()->GetBufferSize();

        const auto ps = params.pixelShader.isEmpty() ? EnginePresetAsset::GetStubPS() : params.pixelShader;
        pipelineDesc.PS.pShaderBytecode = ps.getBlob()->GetBufferPointer();
        pipelineDesc.PS.BytecodeLength = ps.getBlob()->GetBufferSize();

        pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 0xffffffff

        pipelineDesc.BlendState.AlphaToCoverageEnable = false;
        pipelineDesc.BlendState.IndependentBlendEnable = false;

        D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};
        renderTargetBlendDesc.BlendEnable = false;
        renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        renderTargetBlendDesc.LogicOpEnable = false;

        pipelineDesc.BlendState.RenderTarget[0] = renderTargetBlendDesc;

        pipelineDesc.RasterizerState.MultisampleEnable = false;
        pipelineDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        pipelineDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        pipelineDesc.RasterizerState.DepthClipEnable = true;

        pipelineDesc.RasterizerState.FrontCounterClockwise = false;
        pipelineDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        pipelineDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        pipelineDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        pipelineDesc.RasterizerState.AntialiasedLineEnable = false;
        pipelineDesc.RasterizerState.ForcedSampleCount = 0;
        pipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        if (params.hasDepth)
        {
            pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            pipelineDesc.DepthStencilState.DepthEnable = true;
            pipelineDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; // 書き込み可能
            pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // デプステスト
            pipelineDesc.DepthStencilState.StencilEnable = false;
        }

        const auto inputLayout = buildVertexInputLayout(params.vertexInput);
        pipelineDesc.InputLayout.pInputElementDescs = inputLayout.data();
        pipelineDesc.InputLayout.NumElements = static_cast<UINT>(inputLayout.size());

        pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカットなし
        pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 三角形で構成

        pipelineDesc.NumRenderTargets = 1;
        pipelineDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

        pipelineDesc.SampleDesc.Count = 1; // マルチサンプリングなし
        pipelineDesc.SampleDesc.Quality = 0; // クオリティ最低

        m_rootSignature = RootSignature({params.descriptorTable});

        pipelineDesc.pRootSignature = m_rootSignature.getPointer();

        return device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf()));
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::GetCommandList();
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.getPointer());
    }
};

namespace TY
{
    GraphicsPipelineState::GraphicsPipelineState(const PipelineStateParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
        EngineHotReloader::TrackAsset(
            p_impl, {p_impl->m_params.pixelShader.timestamp(), p_impl->m_params.vertexShader.timestamp()});
    }

    DescriptorTable GraphicsPipelineState::descriptorTable() const
    {
        return p_impl ? p_impl->m_params.descriptorTable : DescriptorTable{};
    }

    void GraphicsPipelineState::CommandSet() const
    {
        if (p_impl) p_impl->CommandSet();
    }
}
