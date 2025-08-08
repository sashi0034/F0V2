#pragma once
#include "Color.h"
#include "GraphicsOptions.h"
#include "ScopedDefer.h"
#include "Shader.h"
#include "ShaderResourceTexture.h"
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

        RenderTargetParams& setBufferCount(int bufferCount_);

        RenderTargetParams& setSize(Size size_);

        RenderTargetParams& setClearColor(const ColorF32& clearColor_);

        RenderTargetParams& setFormat(GraphicsFormat format_);
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

        [[nodiscard]]
        int bufferCount() const;

        [[nodiscard]]
        Size size() const;

        void setViewport(const RectF& viewport);

        RectF getViewport() const;

        [[nodiscard]]
        ScopedRenderTarget scopedBind(int index = 0) const;

        [[nodiscard]]
        ShaderResourceTexture asShaderResource(int index = 0) const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
