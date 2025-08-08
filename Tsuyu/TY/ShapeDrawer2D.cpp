#include "pch.h"
#include "ShapeDrawer2D.h"

#include "ArrayPool.h"
#include "ConstantBuffer.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "InlineComponent.h"
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

    GraphicsPipelineStateParams getDefaultPsoParams()
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

    class StateManager
    {
    public:
        struct state_type
        {
            GraphicsPipelineStateParams psoParams{};

            static state_type Default()
            {
                return state_type{
                    .psoParams = getDefaultPsoParams()
                };
            }
        };

        void Reset()
        {
            m_current = state_type::Default();
            m_next.reset();
        }

        void RequestPixelShader(const PixelShader& ps)
        {
            if (m_current.psoParams.shader.ps.unique_id() == ps.unique_id())
            {
                return;
            }

            getNext().psoParams.shader.ps = ps;
        }

        const state_type& Current() const
        {
            return m_current;
        }

        std::optional<state_type> ApplyNext()
        {
            if (m_next.has_value())
            {
                auto previous = std::move(m_current);
                m_current = std::move(m_next.value());
                m_next.reset();
                return previous;
            }

            return std::nullopt;
        }

    private:
        state_type m_current{};

        std::optional<state_type> m_next{};

        state_type& getNext()
        {
            if (m_next.has_value())
            {
                return m_next.value();
            }

            m_next = state_type::Default();
            return m_next.value();
        }
    };
}

struct ShapeDrawer2D::Impl : IEngineDrawer
{
    DescriptorHeap m_descriptorHeap{};

    ShapeBuilder2D::BufferCreator m_bufferCreator{};

    ConstantBuffer<ShapeDraw_b0> m_cb0{};

    ArrayPool<BufferUnit> m_bufferUnitList{};

    StateManager m_stateManager{};

    size_t m_lastTimestamp{}; // TODO: フラッシュのタイムスタンプにする

    size_t m_drawUnitIndex{};

    Impl()
    {
        resetDrawState();

        m_descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {CbvSrvUavSet{{m_cb0}, {}, {}}} // TODO
        });
    }

    void onFlushed() override
    {
        resetDrawState();
    }

    void Push(const Shape2D::shape_type& shape)
    {
        constexpr double maxScaling = 1.0f; // TODO: Transformer の Matrix から取得

        if (shape.isHolds<Shape2D::Rectangle>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildRetangle(m_bufferCreator, shape.get<Shape2D::Rectangle>());
        }
        else if (shape.isHolds<Shape2D::Line>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildLine(m_bufferCreator, shape.get<Shape2D::Line>());
        }
        else if (shape.isHolds<Shape2D::SquareDotLine>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.squareDot);
            applyNextState();
            ShapeBuilder2D::BuildSquareDotLine(m_bufferCreator, shape.get<Shape2D::SquareDotLine>(), maxScaling);
        }
    }

    void Draw()
    {
        flushCurrentBuffer(m_stateManager.Current());

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
    void resetDrawState()
    {
        m_bufferUnitList.logical_resize(0);
        m_lastTimestamp = System::FrameCount();
        m_stateManager.Reset();
        m_drawUnitIndex = 0;
    }

    void applyNextState()
    {
        if (auto&& previous = m_stateManager.ApplyNext())
        {
            flushCurrentBuffer(*previous);
        }
    }

    void flushCurrentBuffer(const StateManager::state_type& state)
    {
        for (const auto& buffer : m_bufferCreator.buffers())
        {
            m_bufferUnitList.add_logical_size(1);
            m_bufferUnitList.logical_back().pso = GraphicsPipelineState{state.psoParams};

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

namespace
{
    struct GlobalInstance : IInlineComponent
    {
        ShapeDrawer2D instance{};
    };

    InlineComponent<GlobalInstance> s_global{};
}

namespace TY
{
    ShapeDrawer2D::ShapeDrawer2D() :
        p_impl(std::make_shared<Impl>())
    {
    }

    const ShapeDrawer2D& ShapeDrawer2D::push(const Shape2D::shape_type& shape) const
    {
        if (not p_impl) return *this;

        p_impl->Push(shape);

        return *this;
    }

    const ShapeDrawer2D& ShapeDrawer2D::operator<<(const Shape2D::shape_type& shape) const
    {
        return push(shape);
    }

    void ShapeDrawer2D::draw() const
    {
        if (p_impl)
        {
            p_impl->Draw();
            EngineRenderContext::MarkDrawerUntilFlush(p_impl);
        }
    }

    ShapeDrawer2D& ShapeDrawer2D::Global()
    {
        return s_global->instance;
    }

    namespace detail
    {
        void InitShapeDrawManager2DComponent()
        {
            EngineComponent::Register<ShapeDrawManager2DComponent>("ShapeDrawManager2DComponent");
        }
    }
}
