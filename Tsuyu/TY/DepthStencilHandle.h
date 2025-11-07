#pragma once

namespace TY
{
    class DepthBufferHandle
    {
    public:
        DepthBufferHandle() = default;

        ID3D12Resource** assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState);

        [[nodiscard]]
        bool isEmpty() const;

        [[nodiscard]]
        ID3D12Resource* getResource() const;

        void transitionResourceState(D3D12_RESOURCE_STATES newState) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
