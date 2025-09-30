#include "pch.h"
#include "GraphicsPipelineState.h"

#include "EngineComponent.h"
#include "TY/AssertObject.h"
#include "EngineHotReloader.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "IEngineHotReloadable.h"
#include "RootSignature.h"
#include "TY/IComponent.h"
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

    D3D12_CULL_MODE getCullMode(GraphicsCullMode mode)
    {
        switch (mode)
        {
        case GraphicsCullMode::None:
            return D3D12_CULL_MODE_NONE;
        case GraphicsCullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case GraphicsCullMode::Back:
            return D3D12_CULL_MODE_BACK;
        default:
            assert(false);
            return {};
        }
    }

    D3D12_FILL_MODE getFillMode(GraphicsFillMode mode)
    {
        switch (mode)
        {
        case GraphicsFillMode::Solid:
            return D3D12_FILL_MODE_SOLID;
        case GraphicsFillMode::Wireframe:
            return D3D12_FILL_MODE_WIREFRAME;
        default:
            assert(false);
            return {};
        }
    }

    D3D12_PRIMITIVE_TOPOLOGY_TYPE getPrimitiveTopology(GraphicsPrimitiveTopology topo)
    {
        switch (topo)
        {
        case GraphicsPrimitiveTopology::TriangleList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case GraphicsPrimitiveTopology::LineList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        default:
            assert(false);
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    }
}

struct GraphicsPipelineState::Impl : IEngineHotReloadable
{
    uint64_t m_timestamp{};

    ComPtr<ID3D12PipelineState> m_pso;
    RootSignature m_rootSignature;
    GraphicsPipelineStateParams m_params;

    Impl(const GraphicsPipelineStateParams& params) :
        m_params(params)
    {
        Impl::HotReload();
    }

    ~Impl()
    {
        DisposeRenderResource();
    }

    uint64_t timestamp() const override
    {
        return m_timestamp;
    }

    void DisposeRenderResource()
    {
        EngineRenderContext::SafeDisposeRenderResource(m_pso);
        EngineRenderContext::SafeDisposeRenderResource(m_rootSignature.get());
    }

    void HotReload() override
    {
        m_timestamp = System::FrameCount();

        DisposeRenderResource();

        if (not m_params.shader.ps.isEmpty() && not m_params.shader.vs.isEmpty())
        {
            if (SUCCEEDED(createPipelineState(m_params)))
            {
                return;
            }
        }

        LogWarning.writeln(L"GraphicsPipelineState Failed to create PSO with user shaders, using stub shaders instead");

        const auto stubParams = makeStubParams(m_params);
        if (SUCCEEDED(createPipelineState(stubParams)))
        {
            return;
        }

        throw std::runtime_error("GraphicsPipelineState: Failed to create PSO.");
    }

    static GraphicsPipelineStateParams makeStubParams(const GraphicsPipelineStateParams& params)
    {
        auto stub = params;
        stub.shader.ps = EnginePresetAsset::GetStubPS();
        stub.shader.vs = EnginePresetAsset::GetStubVS();

        stub.options = GraphicsOptions{};

        return stub;
    }

    HRESULT createPipelineState(const GraphicsPipelineStateParams& params)
    {
        const auto device = EngineRenderContext::GetDevice();
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc = {};

        const auto vs = params.shader.vs;
        pipelineDesc.VS.pShaderBytecode = vs.getBlob()->GetBufferPointer();
        pipelineDesc.VS.BytecodeLength = vs.getBlob()->GetBufferSize();

        const auto ps = params.shader.ps;
        pipelineDesc.PS.pShaderBytecode = ps.getBlob()->GetBufferPointer();
        pipelineDesc.PS.BytecodeLength = ps.getBlob()->GetBufferSize();

        pipelineDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 0xffffffff

        pipelineDesc.BlendState.AlphaToCoverageEnable = false;
        pipelineDesc.BlendState.IndependentBlendEnable = false;

        D3D12_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc = {};

        if (params.options.depth.enable)
        {
            renderTargetBlendDesc.BlendEnable = false;
            renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            renderTargetBlendDesc.LogicOpEnable = false;
        }
        else
        {
            renderTargetBlendDesc.BlendEnable = true;
            renderTargetBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
            renderTargetBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            renderTargetBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
            renderTargetBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
            renderTargetBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
            renderTargetBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
            renderTargetBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        pipelineDesc.BlendState.RenderTarget[0] = renderTargetBlendDesc;

        pipelineDesc.RasterizerState.MultisampleEnable = false;
        pipelineDesc.RasterizerState.CullMode = getCullMode(params.options.rasterizer.cull);
        pipelineDesc.RasterizerState.FillMode = getFillMode(params.options.rasterizer.fill);
        pipelineDesc.RasterizerState.DepthClipEnable = true;

        pipelineDesc.RasterizerState.FrontCounterClockwise = false; // 時計回り (CW) が表
        pipelineDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        pipelineDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        pipelineDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        pipelineDesc.RasterizerState.AntialiasedLineEnable = false;
        pipelineDesc.RasterizerState.ForcedSampleCount = 0;
        pipelineDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        if (params.options.depth.enable)
        {
            pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
            pipelineDesc.DepthStencilState.DepthEnable = true;
            pipelineDesc.DepthStencilState.DepthWriteMask =
                params.options.depth.writeMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            pipelineDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS; // デプステスト
            pipelineDesc.DepthStencilState.StencilEnable = false;
        }

        const auto inputLayout = buildVertexInputLayout(params.vertexInput);
        pipelineDesc.InputLayout.pInputElementDescs = inputLayout.data();
        pipelineDesc.InputLayout.NumElements = static_cast<UINT>(inputLayout.size());

        pipelineDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED; // ストリップ時のカットなし
        pipelineDesc.PrimitiveTopologyType = getPrimitiveTopology(params.options.topology);

        auto rtvFormats = params.options.rtvFormats;
        if (params.options.rtvFormats.size() >= 8)
        {
            LogError(L"Too many render target formats specified. Maximum is 8.");
            rtvFormats.resize(8);
        }

        pipelineDesc.NumRenderTargets = static_cast<UINT>(rtvFormats.size());
        for (size_t i = 0; i < rtvFormats.size(); ++i)
        {
            pipelineDesc.RTVFormats[i] = rtvFormats[i];
        }

        pipelineDesc.SampleDesc.Count = 1; // マルチサンプリングなし
        pipelineDesc.SampleDesc.Quality = 0; // クオリティ最低

        m_rootSignature = RootSignature(RootSignatureParams{
            .samplers = params.options.samplers,
            .descriptorTable = params.descriptorTable,
            .explicitRegisterStarts = params.explicitRegisterStarts
        });

        pipelineDesc.pRootSignature = m_rootSignature.getPointer();

        return device->CreateGraphicsPipelineState(
            &pipelineDesc,
            IID_PPV_ARGS(m_pso.ReleaseAndGetAddressOf()));
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);
        commandList->SetPipelineState(m_pso.Get());
        commandList->SetGraphicsRootSignature(m_rootSignature.getPointer());
    }
};

namespace
{
    size_t combineHash(size_t h1, size_t h2)
    {
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }

    size_t hashParams(const GraphicsPipelineStateParams& params)
    {
        size_t hash = params.descriptorTable.size();
        for (auto& d : params.descriptorTable)
        {
            const size_t h = d.cbvCount << 16 | d.srvCount << 8 | d.uavCount;
            hash = combineHash(hash, h);
        }

        hash = combineHash(hash, params.shader.vs.unique_id());
        hash = combineHash(hash, params.shader.ps.unique_id());
        return hash;
    }
}

class GraphicsPipelineState::Internal
{
public:
    struct GraphicsPipelineStateCacheComponent : IComponent
    {
        bool init() override
        {
            if (s_cache)
            {
                assert(false);
                return false;
            }

            s_cache = this;
            return true;
        }

        ~GraphicsPipelineStateCacheComponent()
        {
            if (s_cache == this)
            {
                s_cache = nullptr;
            }
        }

        bool update() override
        {
            for (auto it = m_cache.begin(); it != m_cache.end();)
            {
                if (it->second.expired())
                {
                    it = m_cache.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            return true;
        }

        std::shared_ptr<Impl> Fetch(const GraphicsPipelineStateParams& params)
        {
            const auto hash = hashParams(params);
            auto range = m_cache.equal_range(hash);

            for (auto it = range.first; it != range.second; ++it)
            {
                if (auto impl = it->second.lock())
                {
                    if (not impl->m_params.equalsTo(params))
                    {
                        continue;
                    }

                    if (impl->m_params.vertexInput != params.vertexInput)
                    {
                        LogError.writeln(
                            L"GraphicsPipelineStateCacheComponent: Vertex input layout mismatch in cached GraphicsPipelineState.");
                    }

                    // LogInfo(std::format(
                    //     "GraphicsPipelineStateCacheComponent: Reusing cached GraphicsPipelineState with hash: 0x{:016x}",
                    //     hash));

                    return impl;
                }
            }

            auto impl = std::make_shared<Impl>(params);

#ifdef _DEBUG
            EngineHotReloader::TrackAsset(
                impl, {impl->m_params.shader.ps.timestamp(), impl->m_params.shader.vs.timestamp()});
#endif

            m_cache.emplace(hash, impl);

            LogInfo(std::format(
                "GraphicsPipelineStateCacheComponent: Created new GraphicsPipelineState with hash: 0x{:016x}",
                hash));

            return impl;
        }

    private:
        std::unordered_multimap<size_t, std::weak_ptr<Impl>> m_cache{};
    };

    static inline GraphicsPipelineStateCacheComponent* s_cache{};
};

namespace TY
{
    bool GraphicsPipelineStateParams::equalsTo(const GraphicsPipelineStateParams& other) const
    {
        // vertexInput はあえて省略 (VS が同じなら vertexInput は等しいはず)

        if (shader.ps.unique_id() != other.shader.ps.unique_id()) return false;
        if (shader.vs.unique_id() != other.shader.vs.unique_id()) return false;
        if (options != other.options) return false;
        if (descriptorTable != other.descriptorTable) return false;
        if (explicitRegisterStarts != other.explicitRegisterStarts) return false;
        return true;
    }

    GraphicsPipelineState::GraphicsPipelineState(const GraphicsPipelineStateParams& params) :
        p_impl(Internal::s_cache->Fetch(params))
    {
    }

    DescriptorTable GraphicsPipelineState::descriptorTable() const
    {
        return p_impl ? p_impl->m_params.descriptorTable : DescriptorTable{};
    }

    void GraphicsPipelineState::commandSet() const
    {
        if (p_impl) p_impl->CommandSet();
    }

    namespace detail
    {
        void InitializeGraphicsPipelineStateCacheComponent()
        {
            EngineComponent::Register<GraphicsPipelineState::Internal::GraphicsPipelineStateCacheComponent>(
                "GraphicsPipelineStateCacheComponent");
        }
    }
}
