#pragma once
#include <d3d12.h>
#include <memory>

namespace TY::detail
{
    struct PlacedBufferAllocatorState;

    class PlacedBufferAllocation
    {
    public:
        PlacedBufferAllocation() = default;
        PlacedBufferAllocation(const PlacedBufferAllocation&) = default;
        PlacedBufferAllocation(PlacedBufferAllocation&&) noexcept = default;
        PlacedBufferAllocation& operator=(const PlacedBufferAllocation& other);
        PlacedBufferAllocation& operator=(PlacedBufferAllocation&& other) noexcept;

        ~PlacedBufferAllocation();

        [[nodiscard]] ID3D12Resource* getResource() const;

        [[nodiscard]] bool isEmpty() const;

        void transitionResourceState(D3D12_RESOURCE_STATES newState) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;

        friend class PlacedBufferAllocator;

    public:
        using Ptr = std::shared_ptr<Impl>;
    };

    // Not thread-safe
    class PlacedBufferAllocator
    {
    public:
        PlacedBufferAllocator(ID3D12Device* device, D3D12_HEAP_TYPE heapType);

        [[nodiscard]] HRESULT createResource(
            const D3D12_RESOURCE_DESC& desc,
            D3D12_RESOURCE_STATES initialState,
            PlacedBufferAllocation& out);

    private:
        std::shared_ptr<PlacedBufferAllocatorState> m_state;
    };

    namespace PlacedBufferAllocator_singleton
    {
        void Init(ID3D12Device* device);

        void Shutdown();

        PlacedBufferAllocator& Default();

        PlacedBufferAllocator& Upload();
    }
}
