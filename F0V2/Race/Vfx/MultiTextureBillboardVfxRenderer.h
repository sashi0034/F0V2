#pragma once

#include "BillboardVfxRenderer.h"

namespace Race
{
    class MultiTextureBillboardVfxRenderer
    {
    public:
        MultiTextureBillboardVfxRenderer() = default;

        void init(
            const Array<ImagePathWrapper>& images,
            int capacity,
            GraphicsBlendOptions blendOptions = GraphicsBlendOptions::AlphaBlend());

        void finalize();

        int capacity() const;

        int textureCount() const;

        void upload(
            const Array<BillboardVfxElement>& elements,
            const Float3& cameraUp,
            const Float3& cameraRight);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
