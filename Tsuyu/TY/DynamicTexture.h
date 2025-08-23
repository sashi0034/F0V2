#pragma once
#include "ImageView.h"
#include "TextureResource.h"

namespace TY
{
    class DynamicTexture
    {
    public:
        DynamicTexture() = default;

        DynamicTexture(const ImageView& image);

        void upload(const ImageView& image);

        TextureResource getResource() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
