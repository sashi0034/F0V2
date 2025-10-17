#pragma once
#include "Color.h"
#include "GraphicsOptions.h"
#include "ScopedDefer.h"
#include "TextureHandle.h"

namespace TY
{
    struct RenderTargetTextureParams
    {
        Size size;

        ColorF32 clearColor;

        GraphicsFormat format{DXGI_FORMAT_R8G8B8A8_UNORM};

        RenderTargetTextureParams& setSize(Size size_);

        RenderTargetTextureParams& setClearColor(const ColorF32& clearColor_);

        RenderTargetTextureParams& setFormat(GraphicsFormat format_);
    };

    class RenderTargetTexture
    {
    public:
        RenderTargetTexture() = default;

        RenderTargetTexture(const RenderTargetTextureParams& params);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        Size size() const;

        [[nodiscard]]
        ColorF32 clearColor() const;

        [[nodiscard]]
        ID3D12Resource* getResource() const;

        [[nodiscard]]
        operator TextureHandle() const;

    protected:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    class UnorderedRenderTargetTexture : public RenderTargetTexture
    {
    public:
        UnorderedRenderTargetTexture() = default;

        UnorderedRenderTargetTexture(const RenderTargetTextureParams& params);

        [[nodiscard]]
        operator UnorderedTextureHandle() const;

        void computeBarrierStart() const;

        void computeBarrierEnd() const;
    };
}
