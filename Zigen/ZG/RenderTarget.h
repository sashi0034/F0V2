#pragma once
#include "Color.h"
#include "ScopedDefer.h"
#include "Shader.h"
#include "Texture.h"
#include "Value2D.h"

namespace ZG
{
    struct RenderTargetParams
    {
        int bufferCount{1};
        Size size;
        ColorF32 clearColor;
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

        [[nodiscard]]
        ScopedRenderTarget scopedBind(int index = 0) const;

        [[nodiscard]]
        ID3D12Resource* getResource(int index = 0) const;

        [[nodiscard]]
        static RenderTarget Current();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
