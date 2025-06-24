#pragma once
#include "ImageView.h"

namespace TY
{
    class DynamicTexture
    {
    public:
        DynamicTexture() = default;

        DynamicTexture(const ImageView& image);

        void upload(const ImageView& image);

        ID3D12Resource* getResource();

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
