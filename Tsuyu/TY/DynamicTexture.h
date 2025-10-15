#pragma once
#include "Image.h"
#include "ImageView.h"
#include "TextureObject.h"

namespace TY
{
    class DynamicTexture
    {
    public:
        DynamicTexture() = default;

        DynamicTexture(const Image& image);

        DynamicTexture(const ImageView& image);

        void upload(const ImageView& image);

        operator TextureObject() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
