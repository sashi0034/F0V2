#include "pch.h"
#include "ShapeDrawer.h"

#include "ArrayPool.h"
#include "ConstantBufferWrapper.h"
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

    struct ShapeDrawerComponent* s_component;

    const std::string shaderPath = "asset/shader/shape2d.hlsl";

    struct ShapeDrawerComponent : IComponent
    {
        struct Subscribable
        {
            virtual ~Subscribable() = default;

            bool m_shouldRemove{};

            virtual void beforeFlush() = 0;

            virtual void afterPresent() = 0;
        };

        VertexShader m_vs{shaderPath, "VS"};

        struct
        {
            PixelShader shape{shaderPath, "PS_Shape"};

            PixelShader squareDot{shaderPath, "PS_SquareDot"};

            PixelShader roundDot{shaderPath, "PS_RoundDot"};

            PixelShader bitmapFont{shaderPath, "PS_BitmapFont"};
        } m_ps{};

        Array<std::shared_ptr<Subscribable>> m_subscribableList{};

        bool init() override
        {
            assert(not s_component);

            s_component = this;

            return true;
        }

        ~ShapeDrawerComponent()
        {
            if (s_component == this)
            {
                s_component = nullptr;
            }
        }

        void beforeFlush() override
        {
            for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
            {
                it->get()->beforeFlush();

                if (it->get()->m_shouldRemove)
                {
                    it = m_subscribableList.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        void afterPresent() override
        {
            for (auto it = m_subscribableList.begin(); it != m_subscribableList.end();)
            {
                it->get()->afterPresent();
                ++it;
            }
        }
    };

    const DescriptorTable descriptorTable = {{1, 0, 0}, {0, 1, 0}};

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
            // .setRasterizer(GraphicsRasterizerOptions().setFill(GraphicsFillMode::Wireframe)),
            .descriptorTable = descriptorTable
        };
    }

    class DescriptorManager
    {
    public:
        struct element_pointer
        {
            int heapIndex{-1};
            int cb0_index{-1};

            bool isValid() const
            {
                return heapIndex >= 0;
            }

            bool operator ==(const element_pointer& other) const
            {
                return std::memcmp(this, &other, sizeof(element_pointer)) == 0;
            }

            bool operator !=(const element_pointer& other) const { return not(*this == other); }
        };

        struct heap_type
        {
            DescriptorHeap descriptorHeap{};

            ConstantBuffer<ShapeDraw_b0> cbv0{};
            Array<ShapeDraw_b0> cb0_value{};
            int next_cb0{};

            struct key_type
            {
                ShaderResourceTexture srv0{};

                bool operator ==(const key_type& other) const
                {
                    return srv0.unique_id() == other.srv0.unique_id();
                }
            } keyResource{};

            bool isFull() const
            {
                return next_cb0 >= cbv0.materialCount();
            }

            void resetSrv0(const ShaderResourceTexture& srv)
            {
                constexpr int tableId = 1;
                keyResource.srv0 = srv;
                descriptorHeap.resetSrv(srv, tableId, 0);
            }

            static constexpr int DefaultCapacity = 4;

            static heap_type Create(const key_type& key, int cb0_capacity = DefaultCapacity)
            {
                heap_type heap{};

                heap.cbv0 = ConstantBuffer<ShapeDraw_b0>(cb0_capacity);
                heap.cb0_value.resize(cb0_capacity);

                heap.keyResource = key;

                heap.descriptorHeap = DescriptorHeap({
                    .table = descriptorTable,
                    .materialCounts = {cb0_capacity, 1},
                    .descriptors = {
                        CbvSrvUavSet{{heap.cbv0}, {}, {}},
                        CbvSrvUavSet{{}, {{heap.keyResource.srv0}}, {}}
                    }
                });

                return heap;
            }
        };

        DescriptorManager()
        {
            pushBackNewHeap(heap_type::key_type{}, heap_type::DefaultCapacity);

            Reset();
        }

        void RequestTransform(const Mat3x2& transform)
        {
            bool isDifferent;
            if (m_currentElement.cb0_index == -1)
            {
                isDifferent = true;
            }
            else
            {
                const auto& current_cb0 = currentHeap().cb0_value[m_currentElement.cb0_index];
                isDifferent =
                    current_cb0.g_transform[0].x != transform._11 ||
                    current_cb0.g_transform[0].y != transform._12 ||
                    current_cb0.g_transform[1].x != transform._21 ||
                    current_cb0.g_transform[1].y != transform._22 ||
                    current_cb0.g_transform[0].z != transform._31 ||
                    current_cb0.g_transform[0].w != transform._32;
            }

            if (isDifferent)
            {
                {
                    auto& heap = currentHeap();
                    if (heap.next_cb0 >= heap.cbv0.materialCount())
                    {
                        m_currentElement = pushBackNewHeap(heap.keyResource, heap.next_cb0 * 2);
                    }

                    heap.next_cb0++;
                }

                auto& heap = currentHeap();
                m_currentElement.cb0_index = heap.next_cb0;
                heap.cb0_value[m_currentElement.cb0_index] = {
                    .g_transform = {
                        {transform._11, transform._12, transform._31, transform._32},
                        {transform._21, transform._22, 0.0f, 1.0f}
                    }
                };
            }
        }

        void RequestSrv0(const ShaderResourceTexture& srv)
        {
            if (currentHeap().keyResource.srv0.unique_id() == srv.unique_id())
            {
                return;
            }

            if (currentHeap().keyResource.srv0.isEmpty())
            {
                currentHeap().resetSrv0(srv);
                return;
            }

            auto newKey = currentHeap().keyResource;
            newKey.srv0 = srv;
            m_currentElement = fetchHeap(newKey);
        }

        void Upload() const
        {
            for (int i = 0; i < m_heapList.size(); ++i)
            {
                auto& heap = m_heapList[i];
                if (heap.next_cb0 > 0)
                {
                    heap.cbv0.upload(heap.cb0_value);
                }
            }
        }

        void Reset()
        {
            m_currentElement = element_pointer{.heapIndex = 0, .cb0_index = -1};

            for (int i = 0; i < m_heapList.size(); ++i)
            {
                m_heapList[i].next_cb0 = 0;
            }
        }

        const element_pointer& Current() const
        {
            return m_currentElement;
        }

        void CommandSet(const element_pointer& element) const
        {
            auto& heap = m_heapList[element.heapIndex];
            heap.descriptorHeap.commandSet(PipelineType::Graphics);
            heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 0, element.cb0_index);
            heap.descriptorHeap.commandSetTable(PipelineType::Graphics, 1);
        }

    private:
        Array<heap_type> m_heapList{};
        element_pointer m_currentElement{};

        heap_type& currentHeap()
        {
            return m_heapList[m_currentElement.heapIndex];
        }

        const heap_type& currentHeap() const
        {
            return m_heapList[m_currentElement.heapIndex];
        }

        element_pointer fetchHeap(const heap_type::key_type& keyResource)
        {
            int next_cb0_capacity = heap_type::DefaultCapacity;
            for (int i = 0; i < m_heapList.size(); ++i)
            {
                if (m_heapList[i].keyResource == keyResource)
                {
                    if (not m_heapList[i].isFull())
                    {
                        return element_pointer{i, m_heapList[i].next_cb0};
                    }
                    else
                    {
                        next_cb0_capacity = Max(next_cb0_capacity, m_heapList[i].next_cb0 * 2);
                    }
                }
            }

            return pushBackNewHeap(keyResource, next_cb0_capacity);
        }

        element_pointer pushBackNewHeap(const heap_type::key_type& keyResource, int cb0_capacity)
        {
            m_heapList.push_back(heap_type::Create(keyResource, cb0_capacity));
            m_heapList.back().keyResource = keyResource;
            return element_pointer{static_cast<int>(m_heapList.size()) - 1, -1};
        }
    };

    class StateManager
    {
    public:
        struct state_type
        {
            GraphicsPipelineStateParams psoParams{};
            DescriptorManager::element_pointer descriptor{};

            static state_type Default()
            {
                return state_type{
                    .psoParams = getDefaultPsoParams(),
                    .descriptor = {}
                };
            }
        };

        void Reset()
        {
            m_current = state_type::Default();
            m_next.reset();
        }

        void RequestDescriptor(const DescriptorManager::element_pointer& descriptor)
        {
            assert(descriptor.isValid());

            if (m_current.descriptor != descriptor)
            {
                getNext().descriptor = descriptor;
            }
        }

        void RequestPixelShader(const PixelShader& ps)
        {
            if (m_current.psoParams.shader.ps.unique_id() != ps.unique_id())
            {
                getNext().psoParams.shader.ps = ps;
            }
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
            m_next->descriptor = m_current.descriptor;
            return m_next.value();
        }
    };

    struct BufferUnit
    {
        GraphicsPipelineState pso{};
        DescriptorManager::element_pointer descriptor{};
        IndexBuffer indexBuffer{Empty};
        VertexBuffer<ShapeBuilder2D::Vertex2D> vertexBuffer{Empty};
        size_t indexCount{0};
    };
}

struct ShapeDrawer::Impl : ShapeDrawerComponent::Subscribable
{
    ShapeBuilder2D::BufferCreator m_bufferCreator{};

    ArrayPool<BufferUnit> m_bufferUnitList{};

    DescriptorManager m_descriptorManager{};

    StateManager m_stateManager{};

    size_t m_drawUnitIndex{};

    Impl()
    {
        resetDrawState();
    }

    ~Impl()
    {
        m_shouldRemove = true;
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
            m_descriptorManager.RequestSrv0(shape.get<Shape2D::Text>().font.fetchAtlasSrv());
        }

        const auto transformMatrix = Mat3x2::Screen(RenderTarget::Current().size()); // TODO: キャッシュ
        m_descriptorManager.RequestTransform(transformMatrix);

        m_stateManager.RequestDescriptor(m_descriptorManager.Current());

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
        else if (shape.isHolds<Shape2D::Path>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildPath(m_bufferCreator, shape.get<Shape2D::Path>());
        }
        else if (shape.isHolds<Shape2D::CyclePath>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.shape);
            applyNextState();
            ShapeBuilder2D::BuildCyclePath(m_bufferCreator, shape.get<Shape2D::CyclePath>());
        }
        else if (shape.isHolds<Shape2D::Text>())
        {
            m_stateManager.RequestPixelShader(s_component->m_ps.bitmapFont);
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
        s_component->m_subscribableList.push_back(p_impl);
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
