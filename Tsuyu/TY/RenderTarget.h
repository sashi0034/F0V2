#pragma once
#include "Color.h"
#include "GraphicsOptions.h"
#include "ScopedDefer.h"
#include "Shader.h"
#include "TextureDrawer.h"
#include "Vector2D.h"

namespace TY
{
    struct RenderTargetParams
    {
        int bufferCount{1};
        Size size;
        ColorF32 clearColor;
        GraphicsFormat format{DXGI_FORMAT_R8G8B8A8_UNORM};
        bool allowUav{};

        RenderTargetParams& setBufferCount(int bufferCount_);

        RenderTargetParams& setSize(Size size_);

        RenderTargetParams& setClearColor(const ColorF32& clearColor_);

        RenderTargetParams& setFormat(GraphicsFormat format_);

        RenderTargetParams& setAllowUav(bool allowUav_);
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

        /** @internal */
        RenderTarget(const RenderTargetParams& params, IDXGISwapChain* swapChain);

        bool isEmpty() const;

        size_t unique_id() const;

        [[nodiscard]]
        int bufferCount() const;

        [[nodiscard]]
        Size size() const;

        void setViewport(const RectF& viewport);

        RectF getViewport() const;

        [[nodiscard]]
        ScopedRenderTarget scopedBind(int index = 0) const;

        // TODO: Enhance
        void computeBarrierStart(int index = 0) const;

        void computeBarrierEnd(int index = 0) const;

        // TODO: Rename to asTexture
        [[nodiscard]]
        TextureHandle asShaderResource(int index = 0) const;

        [[nodiscard]]
        UnorderedTextureHandle asUnorderedTexture(int index = 0) const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
