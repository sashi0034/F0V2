#pragma once

#include "ResourcePathWrapper.h"
#include "TY/Array.h"
#include "TY/Color.h"
#include "TY/Vector2D.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct BillboardVfxRenderElement
    {
        Float3 worldPosition{};

        float rotation{};

        Float2 size{1.0f, 1.0f};

        ColorF32 color{1.0f};
    };

    class BillboardVfxRenderer
    {
    public:
        BillboardVfxRenderer() = default;

        void init(const ImagePathWrapper& image, int capacity);

        void finalize();

        int capacity() const;

        void upload(
            const Array<BillboardVfxRenderElement>& elements,
            const Float3& cameraUp,
            const Float3& cameraRight);

        void draw() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
