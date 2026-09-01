#include "pch.h"
#include "ImmediateDrawer.h"

#include "ArrayPool.h"
#include "DynamicBinding.h"
#include "Graphics3D.h"
#include "InlineComponent.h"
#include "ImmediateBuilder2D.h"
#include "ImmediateBuilder3D.h"
#include "detail/ComponentManager_singleton.h"
#include "detail/RenderContext_singleton.h"
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
        ID_DescriptorManager::element_cursor descriptor{};
        DynamicVertexBufferHandle vertexBuffer{};
        DynamicIndexBufferHandle indexBuffer{};
        bool is3D{};
    };

    PixelShader takeFontPS(const FontObject& font)
    {
        if (font.isBitmap())
        {
            return ImmediateDrawerComponent::Instance->m_ps2d.bitmapFont;
        }
        else if (font.isSdf())
        {
            return ImmediateDrawerComponent::Instance->m_ps2d.sdfFont;
        }
        else
        {
            assert(false);
            return PixelShader{};
        }
    }
}

struct ImmediateDrawer::Impl : RenderEvent::Listener
{
    ImmediateBuilder2D::BufferCreator m_bufferCreator2D{};
    ImmediateBuilder3D::BufferCreator m_bufferCreator3D{};

    ArrayPool<BufferUnit> m_bufferUnitList{};

    ID_DescriptorManager m_descriptorManager{};

    ID_StateManager m_stateManager{};

    size_t m_drawUnitIndex{};

    Impl()
    {
        resetDrawState();
    }

    void afterPresent() override
    {
        resetDrawState();
    }

    // TODO: 複雑になってきたのでリファクタリングしたい

    void Push(const Immediate2D::shape_type& shape)
    {
        constexpr double maxScaling = 1.0f; // TODO: Transformer の Matrix から取得

        if (shape.isHolds<Immediate2D::Texture>())
        {
            m_descriptorManager.RequestSrv0(shape.get<Immediate2D::Texture>().texture);
        }
        else if (shape.isHolds<Immediate2D::Text>())
        {
            m_descriptorManager.RequestSrv0(shape.get<Immediate2D::Text>().font.atlasTexture());
        }
        else if (shape.isHolds<Immediate2D::CachedText>())
        {
            m_descriptorManager.RequestSrv0(shape.get<Immediate2D::CachedText>().font.atlasTexture());
        }

        // -----------------------------------------------

        const auto transformMatrix = Mat3x2::Screen(RenderTarget::Current().size()); // TODO: キャッシュ
        m_descriptorManager.RequestTransform(transformMatrix);
        m_descriptorManager.CommitCurrentHeap();

        // -----------------------------------------------

        m_stateManager.request2D();
        m_stateManager.RequestDescriptor(m_descriptorManager.CurrentCursor(), m_descriptorManager.CurrentHeap().table);

        // -----------------------------------------------

        auto&& component = ImmediateDrawerComponent::Instance;
        if (shape.isHolds<Immediate2D::Rect>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ImmediateBuilder2D::BuildRect(m_bufferCreator2D, shape.get<Immediate2D::Rect>());
        }
        else if (shape.isHolds<Immediate2D::RoundRect>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ImmediateBuilder2D::BuildRoundRect(m_bufferCreator2D, shape.get<Immediate2D::RoundRect>());
        }
        else if (shape.isHolds<Immediate2D::Line>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ImmediateBuilder2D::BuildLine(m_bufferCreator2D, shape.get<Immediate2D::Line>());
        }
        else if (shape.isHolds<Immediate2D::SquareDotLine>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.squareDot);
            commitPendingState();
            ImmediateBuilder2D::BuildSquareDotLine(m_bufferCreator2D, shape.get<Immediate2D::SquareDotLine>(),
                                                   maxScaling);
        }
        else if (shape.isHolds<Immediate2D::Path>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ImmediateBuilder2D::BuildPath(m_bufferCreator2D, shape.get<Immediate2D::Path>());
        }
        else if (shape.isHolds<Immediate2D::CyclePath>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.shape);
            commitPendingState();
            ImmediateBuilder2D::BuildCyclePath(m_bufferCreator2D, shape.get<Immediate2D::CyclePath>());
        }
        else if (shape.isHolds<Immediate2D::Texture>())
        {
            m_stateManager.RequestPixelShader(component->m_ps2d.texture);
            commitPendingState();
            ImmediateBuilder2D::BuildTexture(m_bufferCreator2D, shape.get<Immediate2D::Texture>());
        }
        else if (shape.isHolds<Immediate2D::Text>())
        {
            m_stateManager.RequestPixelShader(takeFontPS(shape.get<Immediate2D::Text>().font));
            commitPendingState();
            ImmediateBuilder2D::BuildText(m_bufferCreator2D, shape.get<Immediate2D::Text>());
        }
        else if (shape.isHolds<Immediate2D::CachedText>())
        {
            m_stateManager.RequestPixelShader(takeFontPS(shape.get<Immediate2D::CachedText>().font));
            commitPendingState();
            ImmediateBuilder2D::BuildCachedText(m_bufferCreator2D, shape.get<Immediate2D::CachedText>());
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
        m_descriptorManager.CommitCurrentHeap();

        m_stateManager.request3D();
        m_stateManager.RequestDescriptor(m_descriptorManager.CurrentCursor(), m_descriptorManager.CurrentHeap().table);

        auto&& component = ImmediateDrawerComponent::Instance;
        if (shape.isHolds<Immediate3D::Line>())
        {
            m_stateManager.RequestPixelShader(component->m_ps3d.shape);
            commitPendingState();
            ImmediateBuilder3D::BuildLine(m_bufferCreator3D, shape.get<Immediate3D::Line>());
        }
        else if (shape.isHolds<Immediate3D::LineSet>())
        {
            m_stateManager.RequestPixelShader(component->m_ps3d.shape);
            commitPendingState();
            ImmediateBuilder3D::BuildLineSet(m_bufferCreator3D, shape.get<Immediate3D::LineSet>());
        }
        else
        {
            assert(false);
        }
    }

    void Draw()
    {
        RenderContext_singleton::RefreshSceneStateIfNeeded();

        flushCurrentBuffer(m_stateManager.Current());

        for (; m_drawUnitIndex < m_bufferUnitList.logical_size(); ++m_drawUnitIndex)
        {
            const auto& buffer = m_bufferUnitList[m_drawUnitIndex];

            buffer.pso.commandSet();

            m_descriptorManager.CommandSet(buffer.descriptor);

            if (buffer.is3D)
            {
                Graphics3D::DrawLines(buffer.vertexBuffer, buffer.indexBuffer);
            }
            else // 2D
            {
                Graphics3D::DrawTriangles(buffer.vertexBuffer, buffer.indexBuffer);
            }
        }
    }

private:
    void resetDrawState()
    {
        m_bufferCreator2D.clear();
        m_bufferCreator3D.clear();
        m_bufferUnitList.logical_resize(0);
        m_descriptorManager.AfterPresent();
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

    void flushCurrentBuffer(const ID_StateManager::state_type& state)
    {
        for (const auto& buffer : m_bufferCreator2D.buffers())
        {
            flushCurrentBuffer_internal<ImmediateBuilder2D::Vertex2D, false>(state, buffer);
        }

        m_bufferCreator2D.clear();

        for (const auto& buffer : m_bufferCreator3D.buffers())
        {
            flushCurrentBuffer_internal<ImmediateBuilder3D::Vertex3D, true>(state, buffer);
        }

        m_bufferCreator3D.clear();
    }

    // using VertexType = ShapeBuilder2D::Vertex2D; // for IDE
    template <typename VertexType, bool is3D>
    void flushCurrentBuffer_internal(
        const ID_StateManager::state_type& state,
        const ShapeBufferCreator<VertexType>::buffer_type& buffer)
    {
        m_bufferUnitList.add_logical_size(1);

        m_bufferUnitList.logical_back().pso = GraphicsPipelineState{state.psoParams};

        m_bufferUnitList.logical_back().descriptor = state.descriptor;
        assert(state.descriptor.isValid());

        // インデックスバッファと頂点バッファにデータをアップロード
        using index_type = typename ShapeBufferCreator<VertexType>::index_type;
        m_bufferUnitList.logical_back().vertexBuffer = DynamicBinding::UploadDynamicVertexBuffer(
            std::span<const VertexType>{buffer.vertices.data(), buffer.vertices.size()});
        m_bufferUnitList.logical_back().indexBuffer = DynamicBinding::UploadDynamicIndexBuffer(
            std::span<const index_type>{buffer.indices.data(), buffer.indices.size()});
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
    const ImmediateBuffer& ImmediateBuffer::append(const Immediate2D::shape_type& shape)
    {
        m_shapes.push_back(shape);
        return *this;
    }

    const ImmediateBuffer& ImmediateBuffer::append(const Immediate3D::shape_type& shape)
    {
        m_shapes.push_back(shape);
        return *this;
    }

    void ImmediateBuffer::pushAuto()
    {
        (void)ImmediateDrawer::Global().push(*this);
        m_shapes.clear();
    }

    const Array<ImmediateBuffer::shape_type>& ImmediateBuffer::shapes() const
    {
        return m_shapes;
    }

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

    const ImmediateDrawer& ImmediateDrawer::push(const ImmediateBuffer& buffer) const
    {
        if (not p_impl)
        {
            return *this;
        }

        for (const auto& shape : buffer.shapes())
        {
            if (shape.isHolds<Immediate2D::shape_type>())
            {
                p_impl->Push(shape.get<Immediate2D::shape_type>());
            }
            else if (shape.isHolds<Immediate3D::shape_type>())
            {
                p_impl->Push(shape.get<Immediate3D::shape_type>());
            }
            else
            {
                assert(false);
            }
        }

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
            ComponentManager_singleton::Register<ImmediateDrawerComponent>("ImmediateDrawerComponent");
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
