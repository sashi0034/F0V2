#include "pch.h"
#include "ModelDrawer.h"

#include "Array.h"
#include "ConstantBufferUploader.h"
#include "Graphics3D.h"
#include "Mat4x4.h"
#include "ModelData.h"
#include "ModelLoader.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineCore.h"
#include "detail/EnginePresetAsset.h"
#include "detail/EngineStateContext.h"
#include "detail/IEngineDrawer.h"
#include "detail/GraphicsPipelineState.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    const DescriptorTable baseDescriptorTable = {{1, 0, 0}, {1, 1, 0}};

    GraphicsPipelineState makePipelineState(const ModelDrawerParams& params)
    {
        auto descriptorTable = baseDescriptorTable;
        if (not params.cb2.isEmpty())
        {
            descriptorTable.push_back({1, 0, 0});;
        }

        // TODO: キャッシュする?
        return GraphicsPipelineState{
            PipelineStateParams{
                .pixelShader = params.ps,
                .vertexShader = params.vs,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .hasDepth = true,
                .descriptorTable = descriptorTable,
            }
        };
    }

    struct SceneState_b0
    {
        Mat4x4 worldMat;
        Mat4x4 viewMat;
        Mat4x4 projectionMat;
    };
}

struct ModelDrawer::Impl : IEngineDrawer
{
    ModelBuffer m_modelBuffer{};

    GraphicsPipelineState m_pipelineState;

    DescriptorHeap m_descriptorHeap{};

    ConstantBufferUploader<SceneState_b0> m_cb0{Empty};

    ConstantBufferUploader_impl m_cb2{Empty};

    Impl(const ModelDrawerParams& params) :
        m_modelBuffer(params.model),
        m_pipelineState(makePipelineState(params)),
        m_cb2(params.cb2)
    {
        m_cb0 = ConstantBufferUploader<SceneState_b0>{1};

        // -----------------------------------------------

        auto descriptorHeapParam = DescriptorHeapParams{
            .table = m_pipelineState.descriptorTable(),
            .materialCounts = {1, m_modelBuffer.materialCount()},
            .descriptors = {
                CbSrUaSet{{m_cb0}, {}, {}},
                CbSrUaSet{{m_modelBuffer.materialCB()}, {m_modelBuffer.materialTextures()}, {}}
            },
        };

        if (not params.cb2.isEmpty())
        {
            descriptorHeapParam.materialCounts.push_back(1);
            descriptorHeapParam.descriptors.push_back(CbSrUaSet{{params.cb2}, {}, {}});
        }

        m_descriptorHeap = DescriptorHeap(descriptorHeapParam);
    }

    void Draw(const Mat4x4& worldMatrix) const
    {
        SceneState_b0 sceneState{};
        sceneState.worldMat = EngineStateContext::ApplyWorldMatrix(worldMatrix).mat;
        sceneState.viewMat = EngineStateContext::GetViewMatrix().mat;
        sceneState.projectionMat = EngineStateContext::GetProjectionMatrix().mat;
        m_cb0.upload(sceneState);

        m_pipelineState.commandSet();

        // カメラ行列設定
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 0);

        if (not m_cb2.isEmpty()) m_descriptorHeap.commandSetTable(PipelineType::Graphics, 2);

        // 形状ごとに描画
        for (size_t shapeId = 0; shapeId < m_modelBuffer.materialCount(); ++shapeId)
        {
            const auto& shape = m_modelBuffer.shapeBuffer().shapes()[shapeId];
            m_descriptorHeap.commandSetTable(PipelineType::Graphics, 1, shape.materialIndex);

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

    ModelDrawerParams& ModelDrawerParams::setCB2(const ConstantBufferUploader_impl& cb2_)
    {
        cb2 = std::move(cb2_);
        return *this;
    }

    ModelDrawer::ModelDrawer(const ModelDrawerParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    void ModelDrawer::draw(const Mat4x4& worldMatrix) const
    {
        if (p_impl)
        {
            p_impl->Draw(worldMatrix);
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }
}
