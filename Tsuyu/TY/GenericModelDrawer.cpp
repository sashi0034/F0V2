#include "pch.h"
#include "GenericModelDrawer.h"

#include "detail/DescriptorHeap.h"
#include "detail/RenderContext_singleton.h"
#include "detail/GraphicsPipelineState.h"
#include "TY/Graphics3D.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct ModelState_b1
    {
        Mat4x4 worldMatrix;
    };
}

struct GenericModelDrawer::Impl
{
    bool m_valid{};

    std::shared_ptr<IGenericModelBuffer> m_modelBuffer{};

    GraphicsPipelineState m_pso{};

    DescriptorHeap m_descriptorHeap{};

    ConstantBuffer<ModelState_b1> m_cb1{};

    int m_tableIndexofCbv10AndLater{-1};

    int m_tableIndexofSrv10AndLater{-1};

    Impl(const GenericModelDrawerParams& params)
        : m_modelBuffer(params.model)
    {
        if (not m_modelBuffer)
        {
            LogError("GenericModelDrawer: Input model is empty.");
            return;
        }

        auto materialSrv = m_modelBuffer->materialSrv();
        const uint32_t srvCountPerMaterial = materialSrv.empty() ? 0 : materialSrv[0].size();

        auto descriptorHeap = DescriptorHeapParams{
            .table = {
                {1, 0, 0}, // [0]
                {1, 0, 0}, // [1]
                {1, srvCountPerMaterial, 0}, // [2]
            },
            .materialCounts = {
                1, // [0]
                1, // [1]
                m_modelBuffer->materialCount(), // [2]
            },
            .descriptors = {
                CbvSrvUavSet{{RenderContext_singleton::GetSceneState3D_CB0()}, {}, {}}, // [0]
                CbvSrvUavSet{{m_cb1}, {}, {}}, // [1]
                CbvSrvUavSet{{m_modelBuffer->materialCbv()}, std::move(materialSrv), {}}, // [2],
            },
        };

        Array<ShaderRegisterStart> explicitRegisterStarts{
            ShaderRegisterStart{static_cast<int>(descriptorHeap.table.size()), 10}
        };

        // 拡張 CBV 設定
        if (params.cbv10AndLater.size() > 0)
        {
            m_tableIndexofCbv10AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back({params.cbv10AndLater.size(), 0, 0});
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{params.cbv10AndLater, {}, {}});
        }

        // 拡張 SRV 設定
        if (params.srv10AndLater.size() > 0)
        {
            m_tableIndexofSrv10AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back({0, params.srv10AndLater.size(), 0});
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{{}, {params.srv10AndLater}, {}});
        }

        m_pso = GraphicsPipelineState{
            GraphicsPipelineStateParams{
                .shader = params.shader,
                .vertexInput = params.vertexInput,
                .options = params.options,
                .descriptorTable = descriptorHeap.table,
                .explicitRegisterStarts = std::move(explicitRegisterStarts)
            }
        };

        m_descriptorHeap = DescriptorHeap(descriptorHeap);

        UploadWorldMatrix(Mat4x4::Identity());

        m_valid = true;
    }

    void UploadWorldMatrix(const Mat4x4& worldMatrix) const
    {
        ModelState_b1 b{};
        b.worldMatrix = worldMatrix;
        m_cb1.upload(b);
    }

    void Draw() const
    {
        RenderContext_singleton::RefreshSceneStateIfNeeded();

        m_pso.commandSet();

        // カメラ行列設定
        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetGraphicsTable(0);
        m_descriptorHeap.commandSetGraphicsTable(1);

        // 拡張 CBV セット
        if (m_tableIndexofCbv10AndLater >= 0)
        {
            m_descriptorHeap.commandSetGraphicsTable(m_tableIndexofCbv10AndLater);
        }

        // 拡張 SRV セット
        if (m_tableIndexofSrv10AndLater >= 0)
        {
            m_descriptorHeap.commandSetGraphicsTable(m_tableIndexofSrv10AndLater);
        }

        // 形状ごとに描画
        for (size_t shapeId = 0; shapeId < m_modelBuffer->materialCount(); ++shapeId)
        {
            const auto& shape = m_modelBuffer->shapeAt(shapeId);
            m_descriptorHeap.commandSetGraphicsTable(2, shape.materialIndex);

            Graphics3D::DrawTriangles(shape.vertexBuffer, shape.indexBuffer);
        }
    }
};

namespace TY
{
    GenericModelDrawerParams& GenericModelDrawerParams::setModel(const std::shared_ptr<IGenericModelBuffer>& model_)
    {
        model = model_;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setVertexInput(const Array<VertexInputElement>& vertexInput_)
    {
        vertexInput = vertexInput_;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setShader(const GraphicsShader& shader_)
    {
        shader = shader_;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setOptions(const GraphicsOptions& options_)
    {
        options = options_;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setCbv10AndLater(const Array<ConstantBufferArrayImpl>& cbv)
    {
        cbv10AndLater = cbv;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setSrv10AndLater(const Array<ShaderResourceType>& srv)
    {
        srv10AndLater = srv;
        return *this;
    }

    GenericModelDrawer::GenericModelDrawer(const GenericModelDrawerParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    const GenericModelDrawer& GenericModelDrawer::uploadWorldMatrix(const Mat4x4& worldMatrix) const
    {
        if (p_impl) p_impl->UploadWorldMatrix(worldMatrix);
        return *this;
    }

    void GenericModelDrawer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
        }
    }
}
