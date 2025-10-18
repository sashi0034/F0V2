#pragma once
#include "Empty.h"
#include "Integer2D.h"

namespace TY
{
    class TextureHandle
    {
    public:
        [[nodiscard]]
        TextureHandle(Empty_t)
        {
        }

        [[nodiscard]]
        TextureHandle();

        [[nodiscard]]
        ID3D12Resource** assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        size_t resource_id() const;

        [[nodiscard]]
        Size size() const;

        [[nodiscard]]
        ID3D12Resource* getResource() const;

        [[nodiscard]]
        D3D12_RESOURCE_STATES getResourceState() const;

        void transitionResourceState(D3D12_RESOURCE_STATES newState) const;

        [[nodiscard]]
        DXGI_FORMAT getFormat() const;

        [[nodiscard]]
        int mipCount() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;

        friend class UnorderedTextureHandle;
    };

    class UnorderedTextureHandle : public TextureHandle
    {
    public:
        using TextureHandle::TextureHandle;

        [[nodiscard]]
        UnorderedTextureHandle(const TextureHandle& handle);
    };
}
