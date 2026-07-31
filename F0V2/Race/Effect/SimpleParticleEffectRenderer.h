#pragma once

#include "ResourcePathWrapper.h"
#include "TY/Array.h"
#include "TY/Color.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct SimpleParticleRenderElement
    {
        Float3 worldPosition{};

        ColorF32 color{1.0f};

        float scale{1.0f};
    };

    class SimpleParticleEffectRenderer
    {
    public:
        SimpleParticleEffectRenderer() = default;

        void init(const ImagePathWrapper& image, int capacity);

        void finalize();

        void upload(
            const Array<SimpleParticleRenderElement>& elements,
            const Float3& cameraUp,
            const Float3& cameraRight);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
