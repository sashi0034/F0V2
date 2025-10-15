#pragma once
#include "Integer2D.h"

namespace TY
{
    class TextureObject
    {
    public:
        TextureObject() = default;

        TextureObject(ID3D12Resource* resource);

        bool isEmpty() const;

        size_t resource_id() const;

        Size size() const;

        ID3D12Resource* getResource() const;

        DXGI_FORMAT getFormat() const;

        int mipCount() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    class UnorderedTextureObject : public TextureObject
    {
    public:
        UnorderedTextureObject() = default;

        UnorderedTextureObject(ID3D12Resource* resource);
    };
}
