#pragma once
#include "ImageView.h"

namespace TY
{
    class DynamicTexture
    {
    public:
        DynamicTexture() = default;

        DynamicTexture(const ImageView& image);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
