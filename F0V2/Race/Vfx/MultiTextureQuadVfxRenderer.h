#pragma once

#include "QuadVfxRenderer.h"

namespace Race
{
    class MultiTextureQuadVfxRenderer
    {
    public:
        MultiTextureQuadVfxRenderer() = default;

        void init(
            const Array<ImagePathWrapper>& images,
            int capacity,
            GraphicsBlendOptions blendOptions = GraphicsBlendOptions::AlphaBlend());

        void finalize();

        int capacity() const;

        int textureCount() const;

        void upload(const Array<QuadVfxElement>& elements);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
