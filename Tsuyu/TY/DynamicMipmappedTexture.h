#pragma once
#include "ImageView.h"
#include "TextureResource.h"

namespace TY
{
    class DynamicMipmappedTexture
    {
    public:
        DynamicMipmappedTexture() = default;

        DynamicMipmappedTexture(const ImageView& image);

        TextureResource getResource() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
