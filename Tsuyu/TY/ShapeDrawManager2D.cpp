#include "pch.h"
#include "ShapeDrawManager2D.h"

#include "ArrayPool.h"
#include "ConstantBuffer.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "ShapeBuilder2D.h"
#include "System.h"
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
        Float4 g_colorMul{1.0f};
        Float4 g_colorAdd{0.0f};
    };

    struct ShapeDrawManager2DComponent* s_component;

    const std::string shaderPath = "asset/shader/shape2d.hlsl";

    struct ShapeDrawManager2DComponent : IComponent
    {
        VertexShader m_vs{shaderPath, "VS"};

        struct
        {
            PixelShader shape{shaderPath, "PS_Shape"};

            PixelShader squareDot{shaderPath, "PS_SquareDot"};

            PixelShader roundDot{shaderPath, "PS_RoundDot"};
        } m_ps{};

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

    const DescriptorTable descriptorTable = {{1, 0, 0}};

    struct BufferUnit
    {
        GraphicsPipelineState pso{};
        IndexBuffer indexBuffer{Empty};
        VertexBuffer<ShapeBuilder2D::Vertex2D> vertexBuffer{Empty};
        size_t indexCount{0};
    };
}

struct ShapeDrawManager2D::Impl : IEngineDrawer
{
    DescriptorHeap m_descriptorHeap{};

    ShapeBuilder2D::BufferCreator m_bufferCreator{};

    ConstantBuffer<ShapeDraw_b0> m_cb0{};

    ArrayPool<BufferUnit> m_bufferUnitList{};

    GraphicsPipelineStateParams m_currentPsoParams{}; // TODO: クラス分離?
    std::optional<GraphicsPipelineStateParams> m_nextPsoParams{};

    size_t m_lastTimestamp{}; // TODO: フラッシュのタイムスタンプにする

    size_t m_drawUnitIndex{};

    Impl()
    {
        m_currentPsoParams = getDefaultPsoParams();

        m_descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {CbvSrvUavSet{{m_cb0}, {}, {}}} // TODO
        });
    }

    void Push(const Shape2D::shape_type& shape)
    {
        if (m_lastTimestamp != System::FrameCount())
        {
            // リセット
            m_bufferUnitList.logical_resize(0);
            m_lastTimestamp = System::FrameCount();
            m_currentPsoParams = getDefaultPsoParams();
            m_drawUnitIndex = 0;
        }

        constexpr double maxScaling = 1.0f; // TODO: Transformer の Matrix から取得

        if (shape.isHolds<Shape2D::Rectangle>())
        {
            requestPixelShader(s_component->m_ps.shape);
            applyNextPsoParams();
            ShapeBuilder2D::BuildRetangle(m_bufferCreator, shape.get<Shape2D::Rectangle>());
        }
        else if (shape.isHolds<Shape2D::Line>())
        {
            requestPixelShader(s_component->m_ps.shape);
            applyNextPsoParams();
            ShapeBuilder2D::BuildLine(m_bufferCreator, shape.get<Shape2D::Line>());
        }
        else if (shape.isHolds<Shape2D::SquareDotLine>())
        {
            requestPixelShader(s_component->m_ps.squareDot);
            applyNextPsoParams();
            ShapeBuilder2D::BuildSquareDotLine(m_bufferCreator, shape.get<Shape2D::SquareDotLine>(), maxScaling);
        }
    }

    void Draw()
    {
        flushCurrentBuffer();

        const auto mat3x2 = Mat3x2::Screen(RenderTarget::Current().size());
        m_cb0->g_transform[0] = {mat3x2._11, mat3x2._12, mat3x2._31, mat3x2._32};
        m_cb0->g_transform[1] = {mat3x2._21, mat3x2._22, 0.0f, 1.0f};
        m_cb0.upload();

        m_descriptorHeap.commandSet();

        for (; m_drawUnitIndex < m_bufferUnitList.logical_size(); ++m_drawUnitIndex)
        {
            const auto& buffer = m_bufferUnitList[m_drawUnitIndex];

            buffer.pso.commandSet();

            m_descriptorHeap.commandSetTable(PipelineType::Graphics, 0);

            Graphics3D::DrawTriangles(buffer.vertexBuffer, buffer.indexBuffer, buffer.indexCount);
        }

        m_lastTimestamp = System::FrameCount();
    }

private:
    static GraphicsPipelineStateParams getDefaultPsoParams()
    {
        return GraphicsPipelineStateParams{
            .shader = GraphicsShader{s_component->m_vs, s_component->m_ps.shape},
            .vertexInput = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT},
            },
            .options = GraphicsOptions(),
            .descriptorTable = descriptorTable
        };
    }

    GraphicsPipelineStateParams& getNextPsoParams()
    {
        if (m_nextPsoParams.has_value())
        {
            return m_nextPsoParams.value();
        }

        m_nextPsoParams = getDefaultPsoParams();
        return m_nextPsoParams.value();
    }

    void requestPixelShader(const PixelShader& ps)
    {
        if (m_currentPsoParams.shader.ps.unique_id() == ps.unique_id())
        {
            return;
        }

        getNextPsoParams().shader.ps = ps;
    }

    void applyNextPsoParams()
    {
        if (m_nextPsoParams.has_value())
        {
            flushCurrentBuffer();
            m_currentPsoParams = std::move(m_nextPsoParams.value());
            m_nextPsoParams.reset();
        }
    }

    void flushCurrentBuffer()
    {
        for (const auto& buffer : m_bufferCreator.buffers())
        {
            m_bufferUnitList.add_logical_size(1);
            m_bufferUnitList.logical_back().pso = GraphicsPipelineState{m_currentPsoParams};

            auto& indexBuffer = m_bufferUnitList.logical_back().indexBuffer;
            auto& vertexBuffer = m_bufferUnitList.logical_back().vertexBuffer;

            // インデックスと頂点バッファのサイズを確認し、必要に応じて再確保
            if (indexBuffer.count() < buffer.indices.size())
            {
                // ここでは、あえて size() ではなく capacity() の値を用いる
                indexBuffer =
                    IndexBuffer(Min<int>(buffer.indices.capacity(), UINT16_MAX));
            }

            if (vertexBuffer.count() < buffer.vertices.size())
            {
                vertexBuffer =
                    VertexBuffer<ShapeBuilder2D::Vertex2D>(Min<int>(buffer.vertices.capacity(), UINT16_MAX));
            }

            // インデックスと頂点バッファにデータをアップロード
            indexBuffer.upload(buffer.indices);
            vertexBuffer.upload(buffer.vertices);
            m_bufferUnitList.logical_back().indexCount = buffer.indices.size();

            m_bufferCreator.clear();
        }
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
            EngineRenderContext::MarkDrawerUntilFlush(p_impl);
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
