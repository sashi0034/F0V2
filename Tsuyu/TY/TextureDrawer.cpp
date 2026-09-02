#include "pch.h"
#include "TextureDrawer.h"

#include <d3d12.h>

#include "Graphics3D.h"
#include "DynamicBinding.h"
#include "IndexBuffer.h"
#include "Mat3x2.h"
#include "Rect.h"
#include "RenderTarget.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineStateContext.h"
#include "detail/GraphicsPipelineState.h"
#include "detail/SceneState3D_singleton.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    struct TextureVertex
    {
        Float3 position;
        Float2 uv;
    };

    class TextureVertexData
    {
    public:
        TextureVertexData()
        {
            Reset();
        }

        void Reset()
        {
            m_vertexes.resize(4);
            TransformPosition({-1.0f, 1.0f}, {1.0f, -1.0f});
            TransformUV({0.0f, 0.0f}, {1.0f, 1.0f});
        }

        void TransformPosition(const Float2& tl, const Float2& br)
        {
            m_vertexes[0].position = {tl.x, tl.y, 0.0f}; // 左下
            m_vertexes[1].position = {tl.x, br.y, 0.0f}; // 左上
            m_vertexes[2].position = {br.x, tl.y, 0.0f}; // 右下
            m_vertexes[3].position = {br.x, br.y, 0.0f}; // 右上
        }

        void TransformUV(const Float2& tl, const Float2& br)
        {
            m_vertexes[0].uv = {tl.x, tl.y}; // 左下
            m_vertexes[1].uv = {tl.x, br.y}; // 左上
            m_vertexes[2].uv = {br.x, tl.y}; // 右下
            m_vertexes[3].uv = {br.x, br.y}; // 右上
        }

        const Array<TextureVertex>& Get() const
        {
            return m_vertexes;
        }

    private:
        Array<TextureVertex> m_vertexes{};
    };

    IndexBuffer makeIndexBuffer()
    {
        return Array<uint16_t>{0, 1, 2, 2, 1, 3};
    }

    const DescriptorTable descriptorTable = {
        DescriptorEntry{
            .srvCount = 1,
        },
    };

    GraphicsPipelineState makePipelineState(const TextureDrawerParams& options)
    {
        const auto graphicsOptions = GraphicsOptions{}
            .setDepth((options.hasDepth ? GraphicsDepthOptions::Default3D() : GraphicsDepthOptions{}));

        return GraphicsPipelineState{
            GraphicsPipelineStateParams{
                .shader = options.shader,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .options = graphicsOptions,
                .descriptorTable = descriptorTable,
                .dynamicDescriptorTable = {
                    DynamicDescriptorEntry{
                        .cbvSlot = 0,
                        .cbvCount = 1,
                    },
                },
            }
        };
    }

    struct SceneState_b0
    {
        Mat4x4 worldMat;
        Mat4x4 viewMat;
        Mat4x4 projectionMat;
    };
}

struct TextureDrawer::Impl
{
    TextureHandle m_srv;

    GraphicsPipelineState m_pso;

    TextureVertexData m_textureVertexData;
    VertexBuffer<TextureVertex> m_vertexBuffer{m_textureVertexData.Get()};
    IndexBuffer m_indexBuffer{makeIndexBuffer()};

    SceneState_b0 m_sceneState{};

    DescriptorHeap m_descriptorHeap{};

    Impl(const TextureDrawerParams& options) :
        m_pso(makePipelineState(options))
    {
        m_srv = TextureHandle{options.texture};

        m_descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {CbvSrvUavSet{{}, {{m_srv}}}}
        });
    }

    void DrawInternal() const
    {
        m_pso.commandSet();

        DynamicBinding::SetDynamicCbv(0, m_sceneState);
        DynamicBinding::FlushAsGraphics(
            m_pso.dynamicBindingRootParameterOffset(),
            m_pso.resolvedDynamicDescriptorTable());

        m_descriptorHeap.commandSet();
        m_descriptorHeap.commandSetGraphicsTable(0);

        Graphics3D::DrawTriangles(m_vertexBuffer, m_indexBuffer);
    }

    void Draw3D()
    {
        m_sceneState.worldMat = SceneState3D_singleton::GetWorldMatrix().mat;
        m_sceneState.viewMat = SceneState3D_singleton::GetViewMatrix().mat;
        m_sceneState.projectionMat = SceneState3D_singleton::GetProjectionMatrix().mat;

        m_textureVertexData.Reset();
        m_vertexBuffer.upload(m_textureVertexData.Get());

        DrawInternal();
    }

    // 2D
    void Draw(const Float2& position, const TextureDrawable2D& drawable)
    {
        const auto mat3x2 = Mat3x2::Screen(RenderTarget::Current().size());
        const auto region = RectF{position, m_srv.size() * drawable.scaling};
        const auto transformedTL = mat3x2.transformPoint(region.tl());
        const auto transformedBR = mat3x2.transformPoint(region.br());
        m_textureVertexData.TransformPosition(transformedTL, transformedBR);
        m_vertexBuffer.upload(m_textureVertexData.Get());

        DrawInternal();
    }

    void DrawAt(const Float2 center, const TextureDrawable2D& drawable)
    {
        const auto size = m_srv.size() * drawable.scaling;
        const auto tl = center - size.cast<double>() / 2.0;
        Draw(tl, drawable);
    }
};

namespace TY
{
    TextureDrawerParams& TextureDrawerParams::setTexture(const TextureHandle& texture_)
    {
        texture = texture_;
        return *this;
    }

    TextureDrawer::TextureDrawer(const TextureDrawerParams& params) :
        p_impl{std::make_shared<Impl>(params)}
    {
    }

    // void Texture::draw(const Vec2& position) const
    // {
    //     if (p_impl)
    //     {
    //         p_impl->Draw(position, {});
    //         EngineRenderContext::MarkDrawerUntilFlush(p_impl);
    //     }
    // }
    //
    // void Texture::drawAt(const Vec2& center) const
    // {
    //     if (p_impl)
    //     {
    //         p_impl->DrawAt(center, {});
    //         EngineRenderContext::MarkDrawerUntilFlush(p_impl);
    //     }
    // }

    TextureDrawable2D TextureDrawer::as2D() const
    {
        return TextureDrawable2D{*this};
    }

    void TextureDrawer::draw3D() const
    {
        if (p_impl)
        {
            p_impl->Draw3D();
        }
    }

    Size TextureDrawer::size() const
    {
        return p_impl ? p_impl->m_srv.size() : Size{};
    }

    TextureDrawable2D& TextureDrawable2D::scaled(float value)
    {
        scaling = Float2{value, value} * scaling;
        return *this;
    }

    TextureDrawable2D& TextureDrawable2D::scaled(Float2 scaling_)
    {
        scaling = scaling * scaling_;
        return *this;
    }

    TextureDrawable2D& TextureDrawable2D::resized(Float2 size)
    {
        scaling = size / texture.size().cast<Float2>();
        return *this;
    }

    void TextureDrawable2D::draw(const Float2& position) const
    {
        if (texture.p_impl)
        {
            texture.p_impl->Draw(position, *this);
        }
    }

    void TextureDrawable2D::drawAt(const Float2& center) const
    {
        if (texture.p_impl)
        {
            texture.p_impl->DrawAt(center, *this);
        }
    }
}
