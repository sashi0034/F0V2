#include "pch.h"
#include "PipelineState.h"

#include "TY/AssertObject.h"
#include "EngineCore.h"
#include "EngineHotReloader.h"
#include "EnginePresetAsset.h"
#include "IEngineHotReloadable.h"
#include "TY/Logger.h"
#include "TY/System.h"
#include "TY/Utils.h"

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

    ComPtr<ID3D12RootSignature> createRootSignature(const Array<DescriptorTableElement>& descriptorTable)
    {
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

        // ディスクリプタテーブルの設定
        std::vector<D3D12_ROOT_PARAMETER> rootParameters{};
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges{};
        int cbvOffset{};
        int srvOffset{};
        int uavOffset{};
        descriptorRanges.resize(descriptorTable.size());
        for (int i = 0; i < descriptorTable.size(); ++i)
        {
            if (descriptorTable[i].cbvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].cbvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                d.BaseShaderRegister = cbvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                cbvOffset += descriptorTable[i].cbvCount;
            }

            if (descriptorTable[i].srvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].srvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                d.BaseShaderRegister = srvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                srvOffset += descriptorTable[i].srvCount;
            }

            if (descriptorTable[i].uavCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].uavCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                d.BaseShaderRegister = uavOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                uavOffset += descriptorTable[i].uavCount;
            }

            D3D12_ROOT_PARAMETER rootParameter = {};
            rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameter.DescriptorTable.pDescriptorRanges = descriptorRanges[i].data();
            rootParameter.DescriptorTable.NumDescriptorRanges = descriptorRanges[i].size();
            rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            rootParameters.push_back(rootParameter);
        }

        rootSignatureDesc.NumParameters = rootParameters.size();
        rootSignatureDesc.pParameters = rootParameters.data();
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // -----------------------------------------------
        // サンプラーの設定
        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 横繰り返し
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 縦繰り返し
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 奥行繰り返し
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーの時は黒
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // ニアレストネイバー
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
        samplerDesc.MinLOD = 0.0f; // ミップマップ最小値
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ?
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダからアクセス

        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &samplerDesc;

        // -----------------------------------------------
        // ルートシグネチャの作成
        ComPtr<ID3D10Blob> rootSignatureBlob{};
        ComPtr<ID3DBlob> errorBlob = nullptr;
        D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &rootSignatureBlob,
            &errorBlob);
        if (errorBlob)
        {
            System::ModalError(StringifyBlob(errorBlob.Get()));
            throw std::runtime_error("failed to serialize root signature");
        }

        ComPtr<ID3D12RootSignature> rootSignature;
        AssertWin32{"failed to create root signature"sv}
            | EngineCore.GetDevice()->CreateRootSignature(
                0,
                rootSignatureBlob->GetBufferPointer(),
                rootSignatureBlob->GetBufferSize(),
                IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf()));
        rootSignatureBlob->Release();

        return rootSignature;
    }
}

struct PipelineState::Impl : IEngineHotReloadable
{
    uint64_t m_timestamp{};

    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;
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

        LogWarning.Writeln(L"failed to create pipeline state with user shaders, using stub shaders instead");

        auto params2 = m_params;
        params2.pixelShader = EnginePresetAsset::GetStubPS();
        params2.vertexShader = EnginePresetAsset::GetStubVS();

        if (SUCCEEDED(createPipelineState(params2))) return;

        throw std::runtime_error("failed to create pipeline state");
    }

    HRESULT createPipelineState(const PipelineStateParams& params)
    {
        const auto device = EngineCore.GetDevice();
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

        m_rootSignature.Reset();
        m_rootSignature = createRootSignature(params.descriptorTable);

        pipelineDesc.pRootSignature = m_rootSignature.Get();

        return device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf()));
    }

    void CommandSet() const
    {
        const auto commandList = EngineCore.GetCommandList();
        commandList->SetPipelineState(m_pipelineState.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    }
};

namespace TY
{
    PipelineState::PipelineState(const PipelineStateParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
        EngineHotReloader.TrackAsset(
            p_impl, {p_impl->m_params.pixelShader.timestamp(), p_impl->m_params.vertexShader.timestamp()});
    }

    DescriptorTable PipelineState::descriptorTable() const
    {
        return p_impl->m_params.descriptorTable;
    }

    void PipelineState::CommandSet() const
    {
        p_impl->CommandSet();
    }
}
