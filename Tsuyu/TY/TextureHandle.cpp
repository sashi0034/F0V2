#include "pch.h"
#include "TextureHandle.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct TextureHandle::Impl
{
    ComPtr<ID3D12Resource> m_textureBuffer{};
    D3D12_RESOURCE_STATES m_resourceState{D3D12_RESOURCE_STATE_COMMON};

    ~Impl()
    {
        EngineRenderContext::SafeDisposeRenderResource(m_textureBuffer);
    }

    void TransitionResourceState(D3D12_RESOURCE_STATES newState)
    {
        if (m_resourceState != newState)
        {
            const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);

            D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_textureBuffer.Get(),
                m_resourceState,
                newState);
            commandList->ResourceBarrier(1, &barrier);

            m_resourceState = newState;
        }
    }
};

namespace TY
{
    TextureHandle::TextureHandle()
        : p_impl(std::make_shared<Impl>())
    {
    }

    ID3D12Resource** TextureHandle::assignResourceAddress(D3D12_RESOURCE_STATES initialResourceState)
    {
        if (not p_impl)
        {
            p_impl = std::make_shared<Impl>();
        }

        p_impl->m_resourceState = initialResourceState;
        return p_impl->m_textureBuffer.ReleaseAndGetAddressOf();
    }

    bool TextureHandle::isEmpty() const
    {
        return p_impl == nullptr || p_impl->m_textureBuffer == nullptr;
    }

    size_t TextureHandle::resource_id() const
    {
        return p_impl ? reinterpret_cast<size_t>(p_impl->m_textureBuffer.Get()) : 0;
    }

    Size TextureHandle::size() const
    {
        if (not p_impl) return Size{};

        const auto desc = p_impl->m_textureBuffer->GetDesc();

        return Size{static_cast<int>(desc.Width), static_cast<int>(desc.Height)};
    }

    ID3D12Resource* TextureHandle::getResource() const
    {
        return p_impl ? p_impl->m_textureBuffer.Get() : nullptr;
    }

    D3D12_RESOURCE_STATES TextureHandle::getResourceState() const
    {
        return p_impl ? p_impl->m_resourceState : D3D12_RESOURCE_STATE_COMMON;
    }

    void TextureHandle::transitionResourceState(D3D12_RESOURCE_STATES newState) const
    {
        if (p_impl)
        {
            p_impl->TransitionResourceState(newState);
        }
    }

    DXGI_FORMAT TextureHandle::getFormat() const
    {
        return p_impl && p_impl->m_textureBuffer ? p_impl->m_textureBuffer->GetDesc().Format : DXGI_FORMAT_UNKNOWN;
    }

    int TextureHandle::mipCount() const
    {
        return p_impl && p_impl->m_textureBuffer ? static_cast<int>(p_impl->m_textureBuffer->GetDesc().MipLevels) : 0;
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
                LogError("TextureResource: Resource is not created with D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS.");
                return false;
            }

            return true;
        }
    }

    UnorderedTextureHandle::UnorderedTextureHandle(const TextureHandle& handle)
    {
        if (not handle.isEmpty() && checkUnorderedAccess(handle.getResource()))
        {
            p_impl = handle.p_impl;
        }
    }
}
