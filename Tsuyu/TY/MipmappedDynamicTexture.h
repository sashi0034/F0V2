#pragma once
#include "ImageView.h"
#include "TextureResource.h"

namespace TY
{
    class MipmappedDynamicTexture
    {
    public:
        MipmappedDynamicTexture() = default;

        MipmappedDynamicTexture(const ImageView& image);

        TextureResource getResource() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
