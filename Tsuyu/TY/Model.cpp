#include "pch.h"
#include "Model.h"

#include "Array.h"
#include "AssertObject.h"
#include "ConstantBufferUploader.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "Mat4x4.h"
#include "ModelData.h"
#include "ModelLoader.h"
#include "Utils.h"
#include "Value2D.h"
#include "Value3D.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineCore.h"
#include "detail/EnginePresetAsset.h"
#include "detail/EngineStateContext.h"
#include "detail/PipelineState.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    const DescriptorTable baseDescriptorTable = {{1, 0, 0}, {1, 1, 0}};

    PipelineState makePipelineState(const ModelParams& params)
    {
        auto descriptorTable = baseDescriptorTable;
        if (not params.cb2.isEmpty())
        {
            descriptorTable.push_back({1, 0, 0});;
        }

        // TODO: キャッシュする?
        return PipelineState{
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

    struct ShapeBuffer
    {
        uint16_t materialIndex;
        VertexBuffer<ModelVertex> vertexBuffer;
        IndexBuffer indexBuffer;
    };
}

struct Model::Impl
{
    ModelData m_modelData{};
    Array<ShapeBuffer> m_shapes{};
    Array<ModelMaterialParameters> m_materials{};

    PipelineState m_pipelineState;

    DescriptorHeap m_descriptorHeap{};

    ConstantBufferUploader<SceneState_b0> m_cb0{Empty};

    ConstantBufferUploader<ModelMaterialParameters> m_cb1{Empty};

    ConstantBufferUploader_impl m_cb2{Empty};

    Impl(const ModelParams& params) :
        m_pipelineState(makePipelineState(params)),
        m_cb2(params.cb2)
    {
        m_modelData = ModelLoader::FromWavefront(params.filename);

        m_shapes.clear();
        for (auto& shape : m_modelData.shapes)
        {
            ShapeBuffer shapeBuffer{};
            shapeBuffer.materialIndex = shape.materialIndex;
            shapeBuffer.vertexBuffer = VertexBuffer(shape.vertexBuffer);
            shapeBuffer.indexBuffer = IndexBuffer{shape.indexBuffer};
            m_shapes.push_back(shapeBuffer);
        }

        // -----------------------------------------------

        m_cb0 = ConstantBufferUploader<SceneState_b0>{1};

        const auto materials = m_modelData.materials.map([](const ModelMaterial& material)
        {
            return material.parameters;
        });

        m_cb1 = ConstantBufferUploader<ModelMaterialParameters>{materials};

        // -----------------------------------------------

        const Array<ShaderResourceTexture> diffuseTextureList =
            m_modelData.materials.map([](const ModelMaterial& material)
            {
                return material.diffuseTexture;
            });

        auto descriptorHeapParam = DescriptorHeapParams{
            .table = m_pipelineState.descriptorTable(),
            .materialCounts = {1, m_modelData.materials.size()},
            .descriptors = {CbSrUaSet{{m_cb0}, {}, {}}, CbSrUaSet{{m_cb1}, {diffuseTextureList}, {}}},
        };

        if (not params.cb2.isEmpty())
        {
            descriptorHeapParam.materialCounts.push_back(1);
            descriptorHeapParam.descriptors.push_back(CbSrUaSet{{params.cb2}, {}, {}});
        }

        m_descriptorHeap = DescriptorHeap(descriptorHeapParam);
    }

    void Draw() const
    {
        SceneState_b0 sceneState{};
        sceneState.worldMat = EngineStateContext::GetWorldMatrix().mat;
        sceneState.viewMat = EngineStateContext::GetViewMatrix().mat;
        sceneState.projectionMat = EngineStateContext::GetProjectionMatrix().mat;
        m_cb0.upload(sceneState);

        m_pipelineState.CommandSet();

        // カメラ行列設定
        m_descriptorHeap.CommandSet();
        m_descriptorHeap.CommandSetTable(0);

        if (not m_cb2.isEmpty()) m_descriptorHeap.CommandSetTable(2);

        // 形状ごとに描画
        for (size_t shapeId = 0; shapeId < m_shapes.size(); ++shapeId)
        {
            m_descriptorHeap.CommandSetTable(1, m_shapes[shapeId].materialIndex);

            Graphics3D::DrawTriangles(m_shapes[shapeId].vertexBuffer, m_shapes[shapeId].indexBuffer);
        }
    }
};

namespace TY
{
    Model::Model(const ModelParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    void Model::draw() const
    {
        if (p_impl) p_impl->Draw();
    }
}
