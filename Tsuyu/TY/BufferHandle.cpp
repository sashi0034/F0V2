#include "pch.h"
#include "BufferHandle.h"

#include "Logger.h"
#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

struct BufferHandle::Impl
{
    ComPtr<ID3D12Resource> m_resourceBuffer{};
    D3D12_RESOURCE_STATES m_resourceState{D3D12_RESOURCE_STATE_COMMON};

    ~Impl()
    {
        RenderContext_singleton::SafeDisposeRenderResource(m_resourceBuffer);
    }

    void TransitionResourceState(D3D12_RESOURCE_STATES newState)
    {
        if (m_resourceState != newState)
        {
            const auto commandList = RenderContext_singleton::TargetCommandList();

            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_resourceBuffer.Get(),
                m_resourceState,
                newState);
            commandList->ResourceBarrier(1, &barrier);

            m_resourceState = newState;
        }
    }
};

namespace TY
{
    BufferHandle::BufferHandle()
        : p_impl(std::make_shared<Impl>())
    {
    }

    ID3D12Resource** BufferHandle::assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState)
    {
        if (not p_impl)
        {
            p_impl = std::make_shared<Impl>();
        }

        p_impl->m_resourceState = initialResourceState;
        return p_impl->m_resourceBuffer.ReleaseAndGetAddressOf();
    }

    bool BufferHandle::isEmpty() const
    {
        return p_impl == nullptr || p_impl->m_resourceBuffer == nullptr;
    }

    size_t BufferHandle::unique_id() const
    {
        return p_impl ? reinterpret_cast<size_t>(p_impl->m_resourceBuffer.Get()) : 0;
    }

    size_t BufferHandle::size() const
    {
        return p_impl && p_impl->m_resourceBuffer ? p_impl->m_resourceBuffer->GetDesc().Width : 0;
    }

    ID3D12Resource* BufferHandle::getResource() const
    {
        return p_impl ? p_impl->m_resourceBuffer.Get() : nullptr;
    }

    D3D12_RESOURCE_STATES BufferHandle::getResourceState() const
    {
        return p_impl ? p_impl->m_resourceState : D3D12_RESOURCE_STATE_COMMON;
    }

    void BufferHandle::transitionResourceState(D3D12_RESOURCE_STATES newState) const
    {
        if (p_impl)
        {
            p_impl->TransitionResourceState(newState);
        }
    }

    namespace
    {
        bool checkUnorderedAccess(ID3D12Resource* resource)
        {
            if (not resource)
            {
                return false;
            }

            const auto desc = resource->GetDesc();
            if (not(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
            {
                LogError(
                    "UnorderedBufferHandle: Resource is not created with D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS.");
                return false;
            }

            return true;
        }
    }

    UnorderedBufferHandle::UnorderedBufferHandle(const BufferHandle& handle)
    {
        if (not handle.isEmpty() && checkUnorderedAccess(handle.getResource()))
        {
            p_impl = handle.p_impl;
        }
    }
}
