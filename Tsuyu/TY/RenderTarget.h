#pragma once
#include "Color.h"
#include "RenderTargetTexture.h"
#include "ScopedDefer.h"
#include "TextureDrawer.h"

namespace TY
{
    using RtvParams = RenderTargetTextureParams;

    struct RenderTargetParams
    {
        TextureHandle rtv{};

        ColorF32 clearColor{};

        RenderTargetParams& setRtvAndClearColor(const RenderTargetTexture& rtv_);

        RenderTargetParams& setRtvAndClearColor(const RtvParams& rtv_);

        RenderTargetParams& setRtvAndClearColor_unsafe(const TextureHandle& rtv_, const ColorF32& clearColor_);
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
        TextureHandle asTexture() const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
