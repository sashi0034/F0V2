#pragma once

#include "ResourcePathWrapper.h"
#include "TY/Array.h"
#include "TY/Color.h"
#include "TY/GraphicsOptions.h"
#include "TY/Quaternion.h"
#include "TY/Vector2D.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct QuadVfxElement
    {
        Float3 worldPosition{};

        Quaternion rotation{};

        Float2 size{1.0f, 1.0f};

        ColorF32 color{1.0f};

        int textureIndex_;
    };

    class QuadVfxRenderer
    {
    public:
        QuadVfxRenderer() = default;

        void init(
            const ImagePathWrapper& image,
            int capacity,
            GraphicsBlendOptions blendOptions = GraphicsBlendOptions::AlphaBlend());

        void finalize();

        int capacity() const;

        void upload(const Array<QuadVfxElement>& elements);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
