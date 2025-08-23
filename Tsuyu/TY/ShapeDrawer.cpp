#include "pch.h"
#include "ShapeDrawer.h"

#include "ArrayPool.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "InlineComponent.h"
#include "ShapeBuilder2D.h"
#include "VertexBuffer.h"
#include "detail/EngineComponent.h"
#include "detail/EngineRenderContext.h"
#include "detail/GraphicsPipelineState.h"
#include "detail/RenderEventComponent.h"
#include "detail/ShapeDrawer_Component.h"
#include "detail/ShapeDrawer_DescriptorManager.h"
#include "detail/ShapeDrawer_StateManager.h"

using namespace TY;
using namespace TY::detail;
using namespace TY::ShapeDrawer_detail;

namespace
{
    struct BufferUnit
    {
        GraphicsPipelineState pso{};
        SD_DescriptorManager::element_pointer descriptor{};
        IndexBuffer indexBuffer{Empty};
        VertexBuffer<ShapeBuilder2D::Vertex2D> vertexBuffer{Empty};
        size_t indexCount{0};
    };
}

struct ShapeDrawer::Impl : RenderEvent::Lister
{
    ShapeBuilder2D::BufferCreator m_bufferCreator{};

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

    void Push(const Shape2D::shape_type& shape)
    {
        constexpr double maxScaling = 1.0f; // TODO: Transformer の Matrix から取得

        if (shape.isHolds<Shape2D::Text>())
        {
            m_descriptorManager.RequestSrv0(shape.get<Shape2D::Text>().font.atlasTexture());
        }

        const auto transformMatrix = Mat3x2::Screen(RenderTarget::Current().size()); // TODO: キャッシュ
        m_descriptorManager.RequestTransform(transformMatrix);

        m_stateManager.RequestDescriptor(m_descriptorManager.CurrentPointer(), m_descriptorManager.CurrentHeap().table);

        auto&& component = ShapeDrawerComponent::Instance;
        if (shape.isHolds<Shape2D::Rectangle>())
        {
            m_stateManager.RequestPixelShader(component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildRetangle(m_bufferCreator, shape.get<Shape2D::Rectangle>());
        }
        else if (shape.isHolds<Shape2D::Line>())
        {
            m_stateManager.RequestPixelShader(component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildLine(m_bufferCreator, shape.get<Shape2D::Line>());
        }
        else if (shape.isHolds<Shape2D::SquareDotLine>())
        {
            m_stateManager.RequestPixelShader(component->m_ps.squareDot);
            applyNextState();
            ShapeBuilder2D::BuildSquareDotLine(m_bufferCreator, shape.get<Shape2D::SquareDotLine>(), maxScaling);
        }
        else if (shape.isHolds<Shape2D::Path>())
        {
            m_stateManager.RequestPixelShader(component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildPath(m_bufferCreator, shape.get<Shape2D::Path>());
        }
        else if (shape.isHolds<Shape2D::CyclePath>())
        {
            m_stateManager.RequestPixelShader(component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildCyclePath(m_bufferCreator, shape.get<Shape2D::CyclePath>());
        }
        else if (shape.isHolds<Shape2D::Text>())
        {
            auto& font = shape.get<Shape2D::Text>().font;
            if (font.isBitmap())
            {
                m_stateManager.RequestPixelShader(component->m_ps.bitmapFont);
            }
            else if (font.isSdf())
            {
                m_stateManager.RequestPixelShader(component->m_ps.sdfFont);
            }
            else
            {
                assert(false);
            }

            applyNextState();
            ShapeBuilder2D::BuildText(m_bufferCreator, shape.get<Shape2D::Text>());
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

            Graphics3D::DrawTriangles(buffer.vertexBuffer, buffer.indexBuffer, buffer.indexCount);
        }
    }

private:
    void resetDrawState()
    {
        m_bufferCreator.clear();
        m_bufferUnitList.logical_resize(0);
        m_descriptorManager.Reset();
        m_stateManager.Reset(m_descriptorManager.CurrentHeap().table);
        m_drawUnitIndex = 0;
    }

    void applyNextState()
    {
        if (auto&& previous = m_stateManager.ApplyNext())
        {
            flushCurrentBuffer(*previous);
        }
    }

    void flushCurrentBuffer(const SD_StateManager::state_type& state)
    {
        for (const auto& buffer : m_bufferCreator.buffers())
        {
            m_bufferUnitList.add_logical_size(1);

            m_bufferUnitList.logical_back().pso = GraphicsPipelineState{state.psoParams};

            m_bufferUnitList.logical_back().descriptor = state.descriptor;
            assert(state.descriptor.isValid());

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
        }

        m_bufferCreator.clear();
    }
};

namespace
{
    struct GlobalInstance : IInlineComponent
    {
        ShapeDrawer instance{};
    };

    InlineComponent<GlobalInstance> s_global{};
}

namespace TY
{
    ShapeDrawer::ShapeDrawer() :
        p_impl(std::make_shared<Impl>())
    {
        RenderEvent::AddLister(p_impl);
    }

    const ShapeDrawer& ShapeDrawer::push(const Shape2D::shape_type& shape) const
    {
        if (not p_impl) return *this;

        p_impl->Push(shape);

        return *this;
    }

    const ShapeDrawer& ShapeDrawer::operator<<(const Shape2D::shape_type& shape) const
    {
        return push(shape);
    }

    void ShapeDrawer::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
        }
    }

    ShapeDrawer& ShapeDrawer::Global()
    {
        return s_global->instance;
    }

    namespace detail
    {
        void InitShapeDrawerComponent()
        {
            EngineComponent::Register<ShapeDrawerComponent>("ShapeDrawerComponent");
        }
    }
}
