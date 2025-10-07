#include "pch.h"
#include "ImmediateDrawer.h"

#include "ArrayPool.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "InlineComponent.h"
#include "ShapeBuilder2D.h"
#include "ShapeBuilder3D.h"
#include "VertexBuffer.h"
#include "detail/EngineComponent.h"
#include "detail/EngineRenderContext.h"
#include "detail/GraphicsPipelineState.h"
#include "detail/RenderEventComponent.h"
#include "detail/ImmediateDrawer_Component.h"
#include "detail/ImmediateDrawer_DescriptorManager.h"
#include "detail/ImmediateDrawer_StateManager.h"

using namespace TY;
using namespace TY::detail;
using namespace TY::ImmediateDrawer_detail;

namespace
{
    struct BufferUnit
    {
        GraphicsPipelineState pso{};
        SD_DescriptorManager::element_cursor descriptor{};
        IndexBuffer indexBuffer{Empty};
        VertexBuffer<ShapeBuilder2D::Vertex2D> vertexBuffer2D{Empty};
        VertexBuffer<ShapeBuilder3D::Vertex3D> vertexBuffer3D{Empty};
        size_t indexCount{0};
        bool is3D{};

        template <bool is3D>
        auto& getVertexBuffer()
        {
            if constexpr (is3D)
            {
                return vertexBuffer3D;
            }
            else
            {
                return vertexBuffer2D;
            }
        }
    };
}

struct ImmediateDrawer::Impl : RenderEvent::Lister
{
    ShapeBuilder2D::BufferCreator m_bufferCreator2D{};
    ShapeBuilder3D::BufferCreator m_bufferCreator3D{};

    ArrayPool<BufferUnit> m_bufferUnitList{};

    SD_DescriptorManager m_descriptorManager{};

    SD_StateManager m_stateManager{};

    size_t m_drawUnitIndex{};

    Impl()
    {
        resetDrawState();
    }

    void beforeFlush() override
    {
        m_descriptorManager.Upload();
    }

    void afterPresent() override
    {
        resetDrawState();
    }

    // TODO: 複雑になってきたのでリファクタリングしたい
    // 最近 ConstantBuffer のフレーム内における複数 upload に対応したのでそれを利用する

    void Push(const Immediate2D::shape_type& shape)
    {
        constexpr double maxScaling = 1.0f; // TODO: Transformer の Matrix から取得

        if (shape.isHolds<Immediate2D::Text>())
        {
            m_descriptorManager.RequestSrv0(shape.get<Immediate2D::Text>().font.atlasTexture());
        }

        const auto transformMatrix = Mat3x2::Screen(RenderTarget::Current().size()); // TODO: キャッシュ
        m_descriptorManager.RequestTransform(transformMatrix);

        m_stateManager.request2D();
        m_stateManager.RequestDescriptor(m_descriptorManager.CurrentCursor(), m_descriptorManager.CurrentHeap().table);

        auto&& component = ImmediateDrawerComponent::Instance;
        if (shape.isHolds<Immediate2D::Rect>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ShapeBuilder2D::BuildRect(m_bufferCreator2D, shape.get<Immediate2D::Rect>());
        }
        else if (shape.isHolds<Immediate2D::RoundRect>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ShapeBuilder2D::BuildRoundRect(m_bufferCreator2D, shape.get<Immediate2D::RoundRect>());
        }
        else if (shape.isHolds<Immediate2D::Line>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ShapeBuilder2D::BuildLine(m_bufferCreator2D, shape.get<Immediate2D::Line>());
        }
        else if (shape.isHolds<Immediate2D::SquareDotLine>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.squareDot);
            commitPendingState();
            ShapeBuilder2D::BuildSquareDotLine(m_bufferCreator2D, shape.get<Immediate2D::SquareDotLine>(), maxScaling);
        }
        else if (shape.isHolds<Immediate2D::Path>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ShapeBuilder2D::BuildPath(m_bufferCreator2D, shape.get<Immediate2D::Path>());
        }
        else if (shape.isHolds<Immediate2D::CyclePath>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ShapeBuilder2D::BuildCyclePath(m_bufferCreator2D, shape.get<Immediate2D::CyclePath>());
        }
        else if (shape.isHolds<Immediate2D::Text>())
        {
            auto& font = shape.get<Immediate2D::Text>().font;
            if (font.isBitmap())
            {
                m_stateManager.RequestPixelShader(component->m_ps2d.bitmapFont);
            }
            else if (font.isSdf())
            {
                m_stateManager.RequestPixelShader(component->m_ps2d.sdfFont);
            }
            else
            {
                assert(false);
            }

            commitPendingState();
            ShapeBuilder2D::BuildText(m_bufferCreator2D, shape.get<Immediate2D::Text>());
        }
        else
        {
            assert(false);
        }
    }

    void Push(const Immediate3D::shape_type& shape)
    {
        const auto transformMatrix = Mat3x2::Screen(RenderTarget::Current().size()); // TODO: キャッシュ
        m_descriptorManager.RequestTransform(transformMatrix);

        m_stateManager.request3D();
        m_stateManager.RequestDescriptor(m_descriptorManager.CurrentCursor(), m_descriptorManager.CurrentHeap().table);

        auto&& component = ImmediateDrawerComponent::Instance;
        if (shape.isHolds<Immediate3D::Line>())
        {
            m_stateManager.RequestPixelShader(component->m_ps3d.shape);
            commitPendingState();
            ShapeBuilder3D::BuildLine(m_bufferCreator3D, shape.get<Immediate3D::Line>());
        }
        else if (shape.isHolds<Immediate3D::LineSet>())
        {
            m_stateManager.RequestPixelShader(component->m_ps3d.shape);
            commitPendingState();
            ShapeBuilder3D::BuildLineSet(m_bufferCreator3D, shape.get<Immediate3D::LineSet>());
        }
        else
        {
            assert(false);
        }
    }

    void Draw()
    {
        flushCurrentBuffer(m_stateManager.Current());

        for (; m_drawUnitIndex < m_bufferUnitList.logical_size(); ++m_drawUnitIndex)
        {
            const auto& buffer = m_bufferUnitList[m_drawUnitIndex];

            buffer.pso.commandSet();

            m_descriptorManager.CommandSet(buffer.descriptor);

            if (buffer.is3D)
            {
                Graphics3D::DrawLines(buffer.vertexBuffer3D, buffer.indexBuffer, buffer.indexCount);
            }
            else
            {
                Graphics3D::DrawTriangles(buffer.vertexBuffer2D, buffer.indexBuffer, buffer.indexCount);
            }
        }
    }

private:
    void resetDrawState()
    {
        m_bufferCreator2D.clear();
        m_bufferCreator3D.clear();
        m_bufferUnitList.logical_resize(0);
        m_descriptorManager.Reset();
        m_stateManager.Reset(m_descriptorManager.CurrentHeap().table);
        m_drawUnitIndex = 0;
    }

    void commitPendingState()
    {
        if (auto&& previous = m_stateManager.CommitPendingState())
        {
            flushCurrentBuffer(*previous);
        }
    }

    void flushCurrentBuffer(const SD_StateManager::state_type& state)
    {
        for (const auto& buffer : m_bufferCreator2D.buffers())
        {
            flushCurrentBuffer_internal<ShapeBuilder2D::Vertex2D, false>(state, buffer);
        }

        m_bufferCreator2D.clear();

        for (const auto& buffer : m_bufferCreator3D.buffers())
        {
            flushCurrentBuffer_internal<ShapeBuilder3D::Vertex3D, true>(state, buffer);
        }

        m_bufferCreator3D.clear();
    }

    // using VertexType = ShapeBuilder2D::Vertex2D; // for IDE
    template <typename VertexType, bool is3D>
    void flushCurrentBuffer_internal(
        const SD_StateManager::state_type& state,
        const ShapeBufferCreator<VertexType>::buffer_type& buffer)
    {
        m_bufferUnitList.add_logical_size(1);

        m_bufferUnitList.logical_back().pso = GraphicsPipelineState{state.psoParams};

        m_bufferUnitList.logical_back().descriptor = state.descriptor;
        assert(state.descriptor.isValid());

        auto& indexBuffer = m_bufferUnitList.logical_back().indexBuffer;
        auto& vertexBuffer = m_bufferUnitList.logical_back().getVertexBuffer<is3D>();

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
                VertexBuffer<VertexType>(Min<int>(buffer.vertices.capacity(), UINT16_MAX));
        }

        // インデックスと頂点バッファにデータをアップロード
        indexBuffer.upload(buffer.indices);
        vertexBuffer.upload(buffer.vertices);
        m_bufferUnitList.logical_back().indexCount = buffer.indices.size();
        m_bufferUnitList.logical_back().is3D = is3D;
    }
};

namespace
{
    struct GlobalInstance : IInlineComponent
    {
        ImmediateDrawer instance{};
    };

    InlineComponent<GlobalInstance> s_global{};
}

namespace TY
{
    ImmediateDrawer::ImmediateDrawer() :
        p_impl(std::make_shared<Impl>())
    {
        RenderEvent::AddLister(p_impl);
    }

    const ImmediateDrawer& ImmediateDrawer::push(const Immediate2D::shape_type& shape) const
    {
        if (not p_impl) return *this;

        p_impl->Push(shape);

        return *this;
    }

    const ImmediateDrawer& ImmediateDrawer::push(const Immediate3D::shape_type& shape) const
    {
        if (not p_impl) return *this;

        p_impl->Push(shape);

        return *this;
    }

    const ImmediateDrawer& ImmediateDrawer::operator<<(const Immediate2D::shape_type& shape) const
    {
        return push(shape);
    }

    const ImmediateDrawer& ImmediateDrawer::operator<<(const Immediate3D::shape_type& shape) const
    {
        return push(shape);
    }

    void ImmediateDrawer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
        }
    }

    ImmediateDrawer& ImmediateDrawer::Global()
    {
        return s_global->instance;
    }

    namespace detail
    {
        void InitImmediateDrawerComponent()
        {
            EngineComponent::Register<ImmediateDrawerComponent>("ImmediateDrawerComponent");
        }
    }

    void operator>>(const Immediate2D::shape_type& shape, const ImmediateDrawer& drawer)
    {
        (void)drawer.push(shape);
    }

    void operator>>(const Immediate3D::shape_type& shape, const ImmediateDrawer& drawer)
    {
        (void)drawer.push(shape);
    }
}
