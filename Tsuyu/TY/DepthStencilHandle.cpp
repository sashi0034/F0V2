#include "pch.h"
#include "DepthStencilHandle.h"

#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

struct DepthBufferHandle::Impl
{
    ComPtr<ID3D12Resource> m_resource{};
    D3D12_RESOURCE_STATES m_resourceState{D3D12_RESOURCE_STATE_DEPTH_WRITE};

    ~Impl()
    {
        RenderContext_singleton::SafeDisposeRenderObject(m_resource);
    }

    void TransitionResourceState(D3D12_RESOURCE_STATES newState)
    {
        if (m_resourceState != newState)
        {
            const auto commandList = RenderContext_singleton::TargetCommandList();

            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_resource.Get(),
                m_resourceState,
                newState);
            commandList->ResourceBarrier(1, &barrier);

            m_resourceState = newState;
        }
    }
};

namespace TY
{
    ID3D12Resource** DepthBufferHandle::assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState)
    {
        if (not p_impl)
        {
            p_impl = std::make_shared<Impl>();
        }

        p_impl->m_resourceState = initialResourceState;
        return p_impl->m_resource.ReleaseAndGetAddressOf();
    }

    bool DepthBufferHandle::isEmpty() const
    {
        return not p_impl || p_impl->m_resource == nullptr;
    }

    ID3D12Resource* DepthBufferHandle::getResource() const
    {
        return p_impl ? p_impl->m_resource.Get() : nullptr;
    }

    void DepthBufferHandle::transitionResourceState(D3D12_RESOURCE_STATES newState) const
    {
        if (p_impl)
        {
            p_impl->TransitionResourceState(newState);
        }
    }
}
