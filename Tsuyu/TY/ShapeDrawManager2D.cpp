#include "pch.h"
#include "ShapeDrawManager2D.h"

#include "ConstantBuffer.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "ShapeBuilder2D.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineComponent.h"
#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"
#include "detail/GraphicsPipelineState.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct ShapeDraw_b0
    {
        Float4 g_transform[2];
    };

    struct ShapeDrawManager2DComponent* s_component;

    struct ShapeDrawManager2DComponent : IComponent
    {
        GraphicsShader m_shader{GraphicsShader::VS_PS("asset/shader/shape2d.hlsl")};

        bool init() override
        {
            assert(not s_component);

            s_component = this;

            return true;
        }

        ~ShapeDrawManager2DComponent()
        {
            if (s_component == this)
            {
                s_component = nullptr;
            }
        }
    };
}

struct ShapeDrawManager2D::Impl : IEngineDrawer
{
    GraphicsPipelineState m_pso{}; // TODO: リストにする
    DescriptorHeap m_descriptorHeap{};

    ShapeBuilder2D::BufferCreator m_bufferCreator{};

    IndexBuffer m_indexBuffer{Empty};
    VertexBuffer<ShapeBuilder2D::Vertex2D> m_vertexBuffer{Empty};

    ConstantBuffer<ShapeDraw_b0> m_cb0{};

    Impl()
    {
        const DescriptorTable descriptorTable = {{1, 0, 0}};

        m_pso = GraphicsPipelineState(
            GraphicsPipelineStateParams{
                .shader = s_component->m_shader,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT},
                },
                .options = GraphicsOptions(),
                .descriptorTable = descriptorTable
            });

        m_descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {CbvSrvUavSet{{m_cb0}, {}, {}}} // TODO
        });
    }

    void Push(const Shape2D::shape_type& shape)
    {
        if (shape.isHolds<Shape2D::Rectangle>())
        {
            ShapeBuilder2D::BuildRetangle(m_bufferCreator, shape.get<Shape2D::Rectangle>());
        }
        else if (shape.isHolds<Shape2D::Line>())
        {
            ShapeBuilder2D::BuildLine(m_bufferCreator, shape.get<Shape2D::Line>());
        }
    }

    void Draw()
    {
        m_pso.commandSet();

        const auto mat3x2 = Mat3x2::Screen(RenderTarget::Current().size());
        m_cb0->g_transform[0] = {mat3x2._11, mat3x2._12, mat3x2._31, mat3x2._32};
        m_cb0->g_transform[1] = {mat3x2._21, mat3x2._22, 0.0f, 1.0f};
        m_cb0.upload();

        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetTable(PipelineType::Graphics, 0);

        const auto commandList = EngineRenderContext::ActiveCommandList();
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const auto& bufferList = m_bufferCreator.buffers();
        if (bufferList.empty())
        {
            return;
        }

        // インデックスと頂点バッファのサイズを確認し、必要に応じて再確保
        if (m_indexBuffer.count() < bufferList[0].indices.size())
        {
            // ここでは、あえて size() ではなく capacity() の値を用いる
            m_indexBuffer =
                IndexBuffer(Min<int>(bufferList[0].indices.capacity(), UINT16_MAX));
        }

        if (m_vertexBuffer.count() < bufferList[0].vertices.size())
        {
            m_vertexBuffer =
                VertexBuffer<ShapeBuilder2D::Vertex2D>(Min<int>(bufferList[0].vertices.capacity(), UINT16_MAX));
        }

        // インデックスと頂点バッファにデータをアップロード
        m_indexBuffer.upload(bufferList[0].indices);
        m_vertexBuffer.upload(bufferList[0].vertices);

        Graphics3D::DrawTriangles(m_vertexBuffer, m_indexBuffer, bufferList[0].indices.size());

        m_bufferCreator.clear();
    }
};

namespace TY
{
    ShapeDrawManager2D::ShapeDrawManager2D() :
        p_impl(std::make_shared<Impl>())
    {
    }

    const ShapeDrawManager2D& ShapeDrawManager2D::push(const Shape2D::shape_type& shape) const
    {
        if (not p_impl) return *this;

        p_impl->Push(shape);

        return *this;
    }

    const ShapeDrawManager2D& ShapeDrawManager2D::operator<<(const Shape2D::shape_type& shape) const
    {
        return push(shape);
    }

    void ShapeDrawManager2D::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }

    namespace detail
    {
        void InitShapeDrawManager2DComponent()
        {
            EngineComponent::Register<ShapeDrawManager2DComponent>("ShapeDrawManager2DComponent");
        }
    }
}
