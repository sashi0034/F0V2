#include "pch.h"
#include "TextureObject.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct TextureObject::Impl
{
    ComPtr<ID3D12Resource> m_textureBuffer{};

    Impl(ID3D12Resource* resource) : m_textureBuffer(resource)
    {
    }

    ~Impl()
    {
        EngineRenderContext::SafeDisposeRenderResource(m_textureBuffer);
    }
};

namespace TY
{
    TextureObject::TextureObject(ID3D12Resource* resource)
        : p_impl(std::make_shared<Impl>(resource))
    {
        if (not p_impl->m_textureBuffer)
        {
            p_impl.reset();
        }
    }

    bool TextureObject::isEmpty() const
    {
        return p_impl == nullptr;
    }

    size_t TextureObject::resource_id() const
    {
        return p_impl ? reinterpret_cast<size_t>(p_impl->m_textureBuffer.Get()) : 0;
    }

    Size TextureObject::size() const
    {
        if (not p_impl) return Size{};

        const auto desc = p_impl->m_textureBuffer->GetDesc();

        return Size{static_cast<int>(desc.Width), static_cast<int>(desc.Height)};
    }

    ID3D12Resource* TextureObject::getResource() const
    {
        return p_impl ? p_impl->m_textureBuffer.Get() : nullptr;
    }

    DXGI_FORMAT TextureObject::getFormat() const
    {
        return p_impl ? p_impl->m_textureBuffer->GetDesc().Format : DXGI_FORMAT_UNKNOWN;
    }

    int TextureObject::mipCount() const
    {
        return p_impl ? static_cast<int>(p_impl->m_textureBuffer->GetDesc().MipLevels) : 0;
    }

    namespace
    {
        ID3D12Resource* checkUnorderedAccess(ID3D12Resource* resource)
        {
            if (not resource)
            {
                return nullptr;
            }

            const auto desc = resource->GetDesc();
            if (not(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
            {
                LogError("TextureResource: Resource is not created with D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS.");
                return nullptr;
            }

            return resource;
        }
    }

    UnorderedTextureObject::UnorderedTextureObject(ID3D12Resource* resource)
        : TextureObject(checkUnorderedAccess(resource))
    {
    }
}
