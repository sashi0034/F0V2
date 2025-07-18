#include "pch.h"
#include "ModelDrawer.h"

#include "Array.h"
#include "ConstantBufferUploader.h"
#include "Graphics3D.h"
#include "Mat4x4.h"
#include "ModelLoader.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineCore.h"
#include "detail/EnginePresetAsset.h"
#include "detail/EngineRenderContext.h"
#include "detail/IEngineDrawer.h"
#include "detail/GraphicsPipelineState.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    const DescriptorTable basicDescriptorTable = {
        {1, 0, 0},
        {1, 0, 0},
        {1, 1, 0},
        {1, 0, 0},
    };

    GraphicsPipelineState makePipelineState(const ModelDrawerParams& params)
    {
        const auto& cbv = params.cb4AndLater;
        const auto& srv = params.sr1AndLater;

        auto descriptorTable = basicDescriptorTable;
        descriptorTable.push_back({cbv.size(), srv.size(), 0});;

        // TODO: キャッシュする?
        return GraphicsPipelineState{
            GraphicsPipelineStateParams{
                .pixelShader = params.ps,
                .vertexShader = params.vs,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .options = params.options,
                .descriptorTable = descriptorTable,
            }
        };
    }

    struct ModelState_b1
    {
        Mat4x4 worldMatrix;
    };
}

struct ModelDrawer::Impl : IEngineDrawer
{
    ModelBuffer m_modelBuffer{};

    GraphicsPipelineState m_pipelineState;

    DescriptorHeap m_descriptorHeap{};

    ConstantBufferUploader<ModelState_b1> m_cb1{1};

    Impl(const ModelDrawerParams& params) :
        m_modelBuffer(params.model),
        m_pipelineState(makePipelineState(params))
    {
        m_descriptorHeap = DescriptorHeap(DescriptorHeapParams{
            .table = m_pipelineState.descriptorTable(),
            .materialCounts = {1, 1, m_modelBuffer.materialCount(), 1, 1},
            .descriptors = {
                CbvSrvUavSet{{EngineRenderContext::GetSceneState3D_CB0()}, {}, {}},
                CbvSrvUavSet{{m_cb1}, {}, {}},
                CbvSrvUavSet{{m_modelBuffer.materialCB()}, {m_modelBuffer.materialTextures()}, {}},
                CbvSrvUavSet{{ConstantBufferUploader_impl{Empty}}, {}, {}},
                CbvSrvUavSet{params.cb4AndLater, params.sr1AndLater.toColumnVector(), {}}
            },
        });
    }

    void UploadWorldMatrix(const Mat4x4& worldMatrix)
    {
        ModelState_b1 b{};
        b.worldMatrix = worldMatrix;
        m_cb1.upload(b);
    }

    void Draw() const
    {
        m_pipelineState.commandSet();

        // カメラ行列設定
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 0);
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 1);

        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 4);

        // 形状ごとに描画
        for (size_t shapeId = 0; shapeId < m_modelBuffer.materialCount(); ++shapeId)
        {
            const auto& shape = m_modelBuffer.shapeBuffer().shapes()[shapeId];
            m_descriptorHeap.commandSetTable(PipelineType::Graphics, 2, shape.materialIndex);

            Graphics3D::DrawTriangles(shape.vertexBuffer, shape.indexBuffer);
        }
    }
};

namespace TY
{
    ModelDrawerParams& ModelDrawerParams::loadModel(const std::string& filename)
    {
        model = ModelLoader::Load(filename);
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setModel(const ModelBuffer& data_)
    {
        model = data_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setShaders(const PixelShader& ps_, const VertexShader& vs_)
    {
        ps = ps_;
        vs = vs_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setShaders(const GraphicsShader& shader)
    {
        ps = shader.ps;
        vs = shader.vs;
        return *this;
    }

#define DEFINE_SET_CB(n) \
ModelDrawerParams& ModelDrawerParams::setCB##n(const ConstantBufferUploader_impl& cb) \
{ \
    cb##n = std::move(cb); \
    return *this; \
}

    ModelDrawerParams& ModelDrawerParams::setOptions(const GraphicsOptions& options_)
    {
        options = options_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setCbv4AndLater(const Array<ConstantBufferUploader_impl>& cbv)
    {
        cb4AndLater = cbv;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setSrv1AndLater(const Array<ShaderResourceType>& srv)
    {
        sr1AndLater = srv;
        return *this;
    }

    ModelDrawer::ModelDrawer(const ModelDrawerParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    const ModelDrawer& ModelDrawer::uploadWorldMatrix(const Mat4x4& worldMatrix) const
    {
        if (p_impl) p_impl->UploadWorldMatrix(worldMatrix);
        return *this;
    }

    void ModelDrawer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }
}
