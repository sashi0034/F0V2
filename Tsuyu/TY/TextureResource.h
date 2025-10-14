#pragma once
#include "TY/Image.h"
#include "TY/TextureSource.h"

namespace TY
{
    class TextureResource
    {
    public:
        TextureResource() = default;

        TextureResource(const TextureSource& source);

        bool isEmpty() const;

        size_t unique_id() const;

        Size size() const;

        ID3D12Resource* getResource() const;

        DXGI_FORMAT getFormat() const;

        int mipCount() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };

    class UnorderedTextureResource : public TextureResource
    {
    public:
        UnorderedTextureResource() = default;

        UnorderedTextureResource(ID3D12Resource* source);
    };
}
