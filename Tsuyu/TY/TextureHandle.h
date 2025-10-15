#pragma once
#include "Integer2D.h"

namespace TY
{
    class TextureHandle
    {
    public:
        TextureHandle() = default;

        TextureHandle(ID3D12Resource* resource);

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

    class UnorderedTextureHandle : public TextureHandle
    {
    public:
        UnorderedTextureHandle() = default;

        UnorderedTextureHandle(ID3D12Resource* resource);
    };
}
