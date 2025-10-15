#pragma once
#include "ImageView.h"
#include "TextureHandle.h"

namespace TY
{
    class MipmappedDynamicTexture
    {
    public:
        MipmappedDynamicTexture() = default;

        MipmappedDynamicTexture(const ImageView& image);

        operator TextureHandle() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
