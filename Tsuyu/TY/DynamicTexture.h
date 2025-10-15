#pragma once
#include "Image.h"
#include "ImageView.h"
#include "TextureHandle.h"

namespace TY
{
    class DynamicTexture
    {
    public:
        DynamicTexture() = default;

        DynamicTexture(const Image& image);

        DynamicTexture(const ImageView& image);

        void upload(const ImageView& image);

        operator TextureHandle() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
