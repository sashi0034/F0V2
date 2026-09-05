#include "pch.h"
#include "GenericModelDrawer.h"

#include "DynamicBinding.h"
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

    Mat4x4 m_worldMatrix{Mat4x4::Identity()};

    int m_tableIndexofCbv10AndLater{-1};

    int m_tableIndexofSrv10AndLater{-1};

    int m_dynamicCbvCount{};

    Impl(const GenericModelDrawerParams& params)
        : m_modelBuffer(params.model)
    {
        if (not m_modelBuffer)
        {
            LogError("GenericModelDrawer: Input model is empty.");
            return;
        }

        auto materialSrv = m_modelBuffer->materialSrv();
        const int srvCountPerMaterial = materialSrv.empty() ? 0 : static_cast<int>(materialSrv[0].size());

        auto descriptorHeap = DescriptorHeapParams{
            .table = {
                DescriptorEntry{
                    .cbvSlot = 2,
                    .cbvCount = 1,
                    .srvSlot = 0,
                    .srvCount = srvCountPerMaterial,
                }, // [0]
            },
            .materialCounts = {
                m_modelBuffer->materialCount(), // [0]
            },
            .descriptors = {
                CbvSrvUavSet{m_modelBuffer->materialCbv(), std::move(materialSrv), {}}, // [0],
            },
        };

        // 拡張 CBV 設定
        if (params.cbv10AndLater.size() > 0)
        {
            m_tableIndexofCbv10AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back(DescriptorEntry{
                .cbvSlot = 10,
                .cbvCount = static_cast<int>(params.cbv10AndLater.size()),
            });
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{{params.cbv10AndLater}, {}, {}});
        }

        // 拡張 SRV 設定
        if (params.srv10AndLater.size() > 0)
        {
            m_tableIndexofSrv10AndLater = static_cast<int>(descriptorHeap.table.size());

            descriptorHeap.table.push_back(DescriptorEntry{
                .srvSlot = 10,
                .srvCount = static_cast<int>(params.srv10AndLater.size()),
            });
            descriptorHeap.materialCounts.push_back(1);
            descriptorHeap.descriptors.push_back(CbvSrvUavSet{{{}}, {params.srv10AndLater}, {}});
        }
        if (params.dynamicCbvCount < 0)
        {
            LogError("GenericModelDrawer: dynamicCbvCount must be non-negative.");
            assert(false);
            return;
        }

        m_dynamicCbvCount = params.dynamicCbvCount;

        auto dynamicDescriptorTable = Array<DynamicDescriptorEntry>{
            DynamicDescriptorEntry{
                .cbvSlot = 0,
                .cbvCount = 2,
            },
        };
        if (m_dynamicCbvCount > 0)
        {
            dynamicDescriptorTable.push_back(DynamicDescriptorEntry{
                .cbvSlot = 10 + static_cast<int>(params.cbv10AndLater.size()),
                .cbvCount = m_dynamicCbvCount,
            });
        }

        m_pso = GraphicsPipelineState{
            GraphicsPipelineStateParams{
                .shader = params.shader,
                .vertexInput = params.vertexInput,
                .options = params.options,
                .descriptorTable = descriptorHeap.table,
                .dynamicDescriptorTable = dynamicDescriptorTable,
            }
        };

        m_descriptorHeap = DescriptorHeap(descriptorHeap);

        UploadWorldMatrix(Mat4x4::Identity());

        m_valid = true;
    }

    void UploadWorldMatrix(const Mat4x4& worldMatrix)
    {
        m_worldMatrix = worldMatrix;
    }

    void Draw() const
    {
        m_pso.commandSet();

        // カメラ行列設定
        DynamicBinding::SetDynamicCbv(
            0,
            RenderContext_singleton::GetSceneStateDynamicCbv());

        const ModelState_b1 modelState{.worldMatrix = m_worldMatrix};
        DynamicBinding::SetDynamicCbv(1, modelState);

        DynamicBinding::FlushAsGraphics(
            m_pso.dynamicBindingRootParameterOffset(),
            m_pso.resolvedDynamicDescriptorTable());

        m_descriptorHeap.commandSet();

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
        for (int shapeId = 0; shapeId < m_modelBuffer->shapeCount(); ++shapeId)
        {
            const auto& shape = m_modelBuffer->shapeAt(shapeId);
            m_descriptorHeap.commandSetGraphicsTable(0, shape.materialIndex);

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

    GenericModelDrawerParams& GenericModelDrawerParams::setCbv10AndLater(const DescriptorList<ConstantBufferImpl>& cbv)
    {
        cbv10AndLater = cbv;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setSrv10AndLater(const DescriptorList<ShaderResourceType>& srv)
    {
        srv10AndLater = srv;
        return *this;
    }

    GenericModelDrawerParams& GenericModelDrawerParams::setDynamicCbvCount(int count)
    {
        if (count < 0)
        {
            LogError("GenericModelDrawerParams::setDynamicCbvCount: count must be non-negative.");
            assert(false);
            count = 0;
        }

        dynamicCbvCount = count;
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
