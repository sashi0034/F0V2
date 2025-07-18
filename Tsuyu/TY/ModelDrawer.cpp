#include "pch.h"
#include "ModelDrawer.h"

#include "Array.h"
#include "ConstantBufferUploader.h"
#include "Graphics3D.h"
#include "Logger.h"
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
    struct ModelState_b1
    {
        Mat4x4 worldMatrix;
    };
}

struct ModelDrawer::Impl : IEngineDrawer
{
    ModelBuffer m_modelBuffer{};

    GraphicsPipelineState m_pipelineState{};

    DescriptorHeap m_descriptorHeap{};

    ConstantBufferUploader<ModelState_b1> m_cb1{1};

    int m_tableIndexofCbv4AndLater{-1};

    int m_materialCountOfCbv4AndLater{};

    int m_tableIndexofSrv1AndLater{-1};

    Impl(const ModelDrawerParams& params)
        : m_modelBuffer(params.model)
    {
        auto descriptorHeap = DescriptorHeapParams{
            .table = {
                {1, 0, 0}, // [0]
                {1, 0, 0}, // [1]
                {1, 1, 0}, // [2]
                {1, 0, 0}, // [3]
            },
            .materialCounts = {
                1, // [0]
                1, // [1]
                m_modelBuffer.materialCount(), // [2]
                1, // [3]
            },
            .descriptors = {
                CbvSrvUavSet{{EngineRenderContext::GetSceneState3D_CB0()}, {}, {}}, // [0]
                CbvSrvUavSet{{m_cb1}, {}, {}}, // [1]
                CbvSrvUavSet{{m_modelBuffer.materialCB()}, {m_modelBuffer.materialTextures()}, {}}, // [2],
                CbvSrvUavSet{{ConstantBufferUploaderCore{Empty}}, {}, {}}, // [3] reserved for b3
            },
        };

        // 拡張 CBV 設定
        if (params.cbv4AndLater.size() > 0)
        {
            m_materialCountOfCbv4AndLater = params.cbv4AndLater[0].materialCount();

            if (m_materialCountOfCbv4AndLater <= 0)
            {
                LogError("ModelDrawer: cbv4AndLater[0] has no material count.");
            }
            else
            {
                m_tableIndexofCbv4AndLater = static_cast<int>(descriptorHeap.table.size());

                descriptorHeap.table.push_back({params.cbv4AndLater.size(), 0, 0});
                descriptorHeap.materialCounts.push_back(m_materialCountOfCbv4AndLater);
                descriptorHeap.descriptors.push_back(CbvSrvUavSet{params.cbv4AndLater, {}, {}});
            }
        }

        // 拡張 SRV 設定
        if (params.srv1AndLater.size() > 0)
        {
            m_tableIndexofSrv1AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back({0, params.srv1AndLater.size(), 0});
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{{}, params.srv1AndLater.toColumnVector(), {}});
        }

        m_pipelineState = GraphicsPipelineState{
            GraphicsPipelineStateParams{
                .shader = params.shader,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .options = params.options,
                .descriptorTable = descriptorHeap.table,
            }
        };

        m_descriptorHeap = DescriptorHeap(descriptorHeap);
    }

    void UploadWorldMatrix(const Mat4x4& worldMatrix)
    {
        ModelState_b1 b{};
        b.worldMatrix = worldMatrix;
        m_cb1.upload(b);
    }

    void Draw(int materialIndexOfCbv4AndLater) const
    {
        m_pipelineState.commandSet();

        // カメラ行列設定
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 0);
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 1);

        // 拡張 CBV セット
        if (m_tableIndexofCbv4AndLater >= 0)
        {
            if (materialIndexOfCbv4AndLater >= m_materialCountOfCbv4AndLater)
            {
                LogError(std::format("ModelDrawer: materialIndexOfCbv4AndLater ({}) is out of range [0, {}]",
                                     materialIndexOfCbv4AndLater, m_materialCountOfCbv4AndLater - 1));
                materialIndexOfCbv4AndLater = m_materialCountOfCbv4AndLater - 1;
            }

            if (materialIndexOfCbv4AndLater >= 0)
            {
                m_descriptorHeap.commandSetTable(
                    PipelineType::Graphics, m_tableIndexofCbv4AndLater, materialIndexOfCbv4AndLater);
            }
        }
        else
        {
            if (materialIndexOfCbv4AndLater > 0)
            {
                LogError("ModelDrawer: materialIndexOfCbv4AndLater is set but cbv4AndLater is not defined.");
            }
        }

        // 拡張 SRV セット
        if (m_tableIndexofSrv1AndLater >= 0)
        {
            m_descriptorHeap.commandSetTable(PipelineType::Graphics, m_tableIndexofSrv1AndLater);
        }

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

    ModelDrawerParams& ModelDrawerParams::setShader(const VertexShader& vs_, const PixelShader& ps_)
    {
        shader.vs = vs_;
        shader.ps = ps_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setShader(const GraphicsShader& shader_)
    {
        shader.ps = shader_.ps;
        shader.vs = shader_.vs;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setOptions(const GraphicsOptions& options_)
    {
        options = options_;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setCbv4AndLater(const Array<ConstantBufferUploaderCore>& cbv)
    {
        cbv4AndLater = cbv;
        return *this;
    }

    ModelDrawerParams& ModelDrawerParams::setSrv1AndLater(const Array<ShaderResourceType>& srv)
    {
        srv1AndLater = srv;
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
            p_impl->Draw(0);
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }

    void ModelDrawer::draw(int materialIndexOfCbv4AndLater) const
    {
        if (p_impl)
        {
            p_impl->Draw(materialIndexOfCbv4AndLater);
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }
}
