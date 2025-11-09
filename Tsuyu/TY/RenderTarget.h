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
        Array<TextureHandle> target_rtvHandles{};

        Array<ColorF32> target_clearColors{};

        RenderTargetParams& setTargetList(const Array<RenderTargetTexture>& list);

        RenderTargetParams& setTarget(const RenderTargetTexture& rtv_);

        RenderTargetParams& setTarget(const RtvParams& rtv_);

        RenderTargetParams& setTarget_unsafe(const TextureHandle& rtv_, const ColorF32& clearColor_);
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

        [[nodiscard]]
        TextureHandle getFrontTarget() const;

        [[nodiscard]]
        DepthBufferHandle getDepthBuffer() const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
