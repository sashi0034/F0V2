#pragma once
#include "TY/Image.h"
#include "TY/TextureSource.h"

namespace TY::detail
{
    class ShaderResourceTexture
    {
    public:
        ShaderResourceTexture() = default;

        ShaderResourceTexture(const TextureSource& source);

        bool isEmpty() const;

        Size size() const;

        ID3D12Resource* getResource() const;

        DXGI_FORMAT getFormat() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
