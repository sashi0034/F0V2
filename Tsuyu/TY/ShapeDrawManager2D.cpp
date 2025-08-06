#include "pch.h"
#include "ShapeDrawManager2D.h"

#include "ConstantBuffer.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "ShapeBuilder2D.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
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
}

struct ShapeDrawManager2D::Impl : IEngineDrawer
{
    GraphicsShader m_shader{GraphicsShader::VS_PS("asset/shader/shape2d.hlsl")}; // TODO: Component 化

    GraphicsPipelineState m_pso{}; // TODO: リストにする
    DescriptorHeap m_descriptorHeap{};

    ShapeBuilder2D::BufferCreator m_bufferCreator{};

    IndexBuffer m_indexBuffer{99}; // TODO: 動的に変更
    VertexBuffer<ShapeBuilder2D::Vertex2D> m_vertexBuffer{99};

    ConstantBuffer<ShapeDraw_b0> m_cb0{};

    Impl()
    {
        const DescriptorTable descriptorTable = {{1, 0, 0}};

        m_pso = GraphicsPipelineState(
            GraphicsPipelineStateParams{
                .shader = m_shader,
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

        m_indexBuffer.upload(bufferList[0].indices);
        m_vertexBuffer.upload(bufferList[0].vertices);

        Graphics3D::DrawTriangles(m_vertexBuffer, m_indexBuffer);

        m_bufferCreator.clear();
    }
};

namespace TY
{
    ShapeDrawManager2D::ShapeDrawManager2D() :
        p_impl(std::make_shared<Impl>())
    {
    }

    ShapeDrawManager2D& ShapeDrawManager2D::push(const Shape2D::shape_type& shape)
    {
        if (not p_impl) return *this;

        if (shape.isHolds<Shape2D::Rectangle>())
        {
            ShapeBuilder2D::BuildRetangle(p_impl->m_bufferCreator, shape.get<Shape2D::Rectangle>());
        }

        return *this;
    }

    void ShapeDrawManager2D::draw()
    {
        if (p_impl)
        {
            p_impl->Draw();
            EngineCore::MarkDrawerInFrame(p_impl);
        }
    }
}
