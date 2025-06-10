#include "pch.h"
#include "Texture.h"

#include <d3d12.h>

#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "Mat3x2.h"
#include "Rect.h"
#include "RenderTarget.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineStateContext.h"
#include "detail/PipelineState.h"

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

    const DescriptorTable descriptorTable = {{1, 1, 0}};

    PipelineState makePipelineState(const TextureParams& options)
    {
        // TODO: キャッシュする?
        return PipelineState{
            PipelineStateParams{
                .pixelShader = options.ps,
                .vertexShader = options.vs,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .descriptorTable = descriptorTable,
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

struct Texture::Impl
{
    ShaderResourceTexture m_sr;

    PipelineState m_pipelineState;

    TextureVertexData m_textureVertexData;
    VertexBuffer<TextureVertex> m_vertexBuffer{m_textureVertexData.Get()};
    IndexBuffer m_indexBuffer{makeIndexBuffer()};

    ComPtr<ID3D12Resource> m_constantBuffer{};

    ConstantBufferUploader<SceneState_b0> m_cb0{Empty};

    DescriptorHeap m_descriptorHeap{};

    Impl(const TextureParams& options) :
        m_pipelineState(makePipelineState(options))
    {
        m_sr = ShaderResourceTexture{options.source};

        m_cb0 = ConstantBufferUploader<SceneState_b0>{1};

        m_descriptorHeap = DescriptorHeap({
            .table = descriptorTable,
            .materialCounts = {1},
            .descriptors = {CbSrUaSet{{m_cb0}, {{m_sr}}}}
        });
    }

    void DrawInternal() const
    {
        m_pipelineState.CommandSet();

        m_descriptorHeap.CommandSet();
        m_descriptorHeap.CommandSetTable(0);

        Graphics3D::DrawTriangles(m_vertexBuffer, m_indexBuffer);
    }

    void Draw3D()
    {
        SceneState_b0 sceneState{};
        sceneState.worldMat = EngineStateContext::GetWorldMatrix().mat;
        sceneState.viewMat = EngineStateContext::GetViewMatrix().mat;
        sceneState.projectionMat = EngineStateContext::GetProjectionMatrix().mat;
        m_cb0.upload(sceneState);

        m_textureVertexData.Reset();
        m_vertexBuffer.upload(m_textureVertexData.Get());

        DrawInternal();
    }

    // 2D
    void Draw(const RectF& region)
    {
        const auto mat3x2 = Mat3x2::Screen(RenderTarget::Current().size());
        const auto transformedTL = mat3x2.transformPoint(region.tl());
        const auto transformedBR = mat3x2.transformPoint(region.br());
        m_textureVertexData.TransformPosition(transformedTL, transformedBR);
        m_vertexBuffer.upload(m_textureVertexData.Get());

        DrawInternal();
    }

    void DrawAt(const Vec2 position)
    {
        const auto size = m_sr.size();
        const auto tl = position - size.cast<double>() / 2.0;
        Draw(RectF{tl, size.cast<double>()});
    }
};

namespace TY
{
    Texture::Texture(const TextureParams& params) :
        p_impl{std::make_shared<Impl>(params)}
    {
    }

    void Texture::draw(const RectF& region) const
    {
        if (p_impl) p_impl->Draw(region);
    }

    void Texture::drawAt(const Vec2& position) const
    {
        if (p_impl) p_impl->DrawAt(position);
    }

    void Texture::draw3D() const
    {
        if (p_impl) p_impl->Draw3D();
    }
}
