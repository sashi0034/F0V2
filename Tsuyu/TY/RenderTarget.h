#pragma once
#include "Color.h"
#include "DepthStencilHandle.h"
#include "RenderTargetTexture.h"
#include "ScopedDefer.h"
#include "TextureDrawer.h"

namespace TY
{
    using RtvParams = RenderTargetTextureParams;

    struct RenderTargetParams
    {
        Array<TextureHandle> rtvHandles{};

        Array<ColorF32> rtvClearColors{};

        DepthBufferHandle depthBufferOpt{};

        RenderTargetParams& setRtvList(const Array<RenderTargetTexture>& list);

        RenderTargetParams& setRtv(const RenderTargetTexture& rtv_);

        RenderTargetParams& setRtv(const RtvParams& rtv_);

        RenderTargetParams& setRtv_unsafe(const TextureHandle& rtv_, const ColorF32& clearColor_);

        RenderTargetParams& setDepthBuffer(const DepthBufferHandle& depthBuffer_);
    };

    class ScopedRenderTarget : public ScopedDefer
    {
    public:
        using ScopedDefer::ScopedDefer;
    };

    class RenderTarget
    {
    public:
        RenderTarget() = default;

        RenderTarget(const RenderTargetParams& params);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        size_t unique_id() const;

        [[nodiscard]]
        Size size() const;

        void setViewport(const RectF& viewport);

        [[nodiscard]]
        RectF getViewport() const;

        [[nodiscard]]
        ScopedRenderTarget scopedBind() const;

        // TODO: Rename?
        [[nodiscard]]
        TextureHandle getFrontRtv() const;

        [[nodiscard]]
        DepthBufferHandle getDepthBuffer() const;

        [[nodiscard]]
        Array<GraphicsFormat> getRtvFormats() const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
