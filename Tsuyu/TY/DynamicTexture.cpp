#include "pch.h"
#include "DynamicTexture.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct DynamicTexture::Impl
{
    bool m_valid{};

    DXGI_FORMAT m_format{};
    Size m_size{};

    ComPtr<ID3D12Resource> m_textureBuffer{};
    ComPtr<ID3D12Resource> m_uploadBuffer{};

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
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                nullptr,
                IID_PPV_ARGS(&m_textureBuffer));
            FAILED(hr))
        {
            LogError(std::format("DynamicTexture: Failed to create texture buffer: {}", static_cast<int>(hr)));
            return;
        }

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_textureBuffer.Get(), 0, 1);
        CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

        if (FAILED(EngineRenderContext::GetDevice()->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_uploadBuffer))))
        {
            LogError("DynamicTexture: Failed to create upload buffer.");
            return;
        }

        Upload(image);

        m_format = resourceDesc.Format;

        m_size = Size{static_cast<int>(resourceDesc.Width), static_cast<int>(resourceDesc.Height)};

        m_valid = true;
    }

    void Upload(const ImageView& image)
    {
        assert(m_textureBuffer && m_uploadBuffer);

        if (image.size != m_size || image.format != m_format)
        {
            LogError("DynamicTexture: Image size or format does not match the texture buffer.");
            return;
        }

        D3D12_SUBRESOURCE_DATA subresourceData{};
        subresourceData.pData = image.getPointer();
        subresourceData.RowPitch = image.size.x * image.pixelSizeInBytes();
        subresourceData.SlicePitch = subresourceData.RowPitch * image.size.y;

        // const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Copy);
        ID3D12GraphicsCommandList* commandList = EngineRenderContext::ActiveCommandList();

        const CD3DX12_RESOURCE_BARRIER barrierBefore = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textureBuffer.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST);
        commandList->ResourceBarrier(1, &barrierBefore);

        UpdateSubresources(
            commandList,
            m_textureBuffer.Get(),
            m_uploadBuffer.Get(),
            0,
            0,
            1,
            &subresourceData);

        const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textureBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->ResourceBarrier(1, &barrier);
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

    void DynamicTexture::upload(const ImageView& image)
    {
        if (p_impl)
        {
            p_impl->Upload(image);
        }
    }

    ID3D12Resource* DynamicTexture::getResource()
    {
        return p_impl ? p_impl->m_textureBuffer.Get() : nullptr;
    }
}
