#pragma once
#include "Empty.h"
#include "Integer2D.h"

namespace TY
{
    class BufferHandle
    {
    public:
        [[nodiscard]]
        BufferHandle(Empty_t)
        {
        }

        [[nodiscard]]
        BufferHandle();

        [[nodiscard]]
        ID3D12Resource** assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        size_t unique_id() const;

        [[nodiscard]]
        size_t size() const;

        [[nodiscard]]
        ID3D12Resource* getResource() const;

        [[nodiscard]]
        D3D12_RESOURCE_STATES getResourceState() const;

        void transitionResourceState(D3D12_RESOURCE_STATES newState) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;

        friend class UnorderedBufferHandle;
    };

    class UnorderedBufferHandle : public BufferHandle
    {
    public:
        using BufferHandle::BufferHandle;

        [[nodiscard]]
        UnorderedBufferHandle(const BufferHandle& handle);
    };
}
