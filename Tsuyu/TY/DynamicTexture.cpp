#include "pch.h"
#include "DynamicTexture.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct DynamicTexture::Impl
{
    bool m_valid{};

    ComPtr<ID3D12Resource> m_textureBuffer{};
    DXGI_FORMAT m_format{};
    Size m_size{};

    Impl(const ImageView& image)
    {
        D3D12_HEAP_PROPERTIES heapProperties{};
        heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
        heapProperties.CreationNodeMask = 0;
        heapProperties.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = image.size.x;
        resourceDesc.Height = image.size.y;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = image.format;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_textureBuffer));
            FAILED(hr))
        {
            LogError(std::format("DynamicTexture: Failed to create texture buffer: {}", static_cast<int>(hr)));
            return;
        }

        if (const HRESULT hr = m_textureBuffer->WriteToSubresource(
                0,
                nullptr, // リソース全体領域をコピー
                image.getPointer(),
                image.size.x * image.pixelSizeInBytes(),
                image.sizeInBytes);
            FAILED(hr))
        {
            LogError(std::format("DynamicTexture: Failed to write to subresource: {}", static_cast<int>(hr)));
            return;
        }

        m_format = resourceDesc.Format;

        m_size = Size{static_cast<int>(resourceDesc.Width), static_cast<int>(resourceDesc.Height)};

        m_valid = true;
    }
};

namespace TY
{
    DynamicTexture::DynamicTexture(const ImageView& image)
        : p_impl(std::make_shared<Impl>(image))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }
}
