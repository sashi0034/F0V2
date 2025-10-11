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

    ComPtr<ID3D12Resource> m_finalBuffer{};

    TextureResource m_textureResource{};

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
        uint8_t* dest{};
    };

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

    Impl(const ImageView& image)
    {
        m_format = image.format;

        m_size = image.size;

        D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

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
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_finalBuffer));
            FAILED(hr))
        {
            LogError(std::format("DynamicTexture: Failed to create m_finalBuffer: {}", static_cast<int>(hr)));
            return;
        }

        m_finalBuffer->SetName(L"DynamicTexture::m_finalBuffer");

        m_textureResource = TextureResource{m_finalBuffer.Get()};

        Upload(image);

        m_valid = true;
    }

    Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (frameResource.uploadBuffer && frameResource.dest)
            {
                frameResource.uploadBuffer->Unmap(0, nullptr);
            }

            EngineRenderContext::SafeDisposeRenderResource(frameResource.uploadBuffer);
        }

        EngineRenderContext::SafeDisposeRenderResource(m_finalBuffer);
    }

    void Upload(const ImageView& image)
    {
        if (image.size != m_size || image.format != m_format)
        {
            LogError("DynamicTexture: Image size or format does not match the texture buffer.");
            return;
        }

        m_uploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (not frameResource.uploadBuffer)
        {
            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_finalBuffer.Get(), 0, 1);

            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            if (FAILED(EngineRenderContext::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&frameResource.uploadBuffer))))
            {
                LogError("DynamicTexture: Failed to create uploadBuffer.");
                return;
            }

            frameResource.uploadBuffer->SetName(L"DynamicTexture::uploadBuffer");

            // -----------------------------------------------

            if (const HRESULT hr = frameResource.uploadBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.dest));
                FAILED(hr))
            {
                LogError.writeln(std::format("DynamicTexture: Failed to map uploadBuffer"));
                return;
            }
        }

        // -----------------------------------------------
        // footprint

        ID3D12Device* device = EngineRenderContext::GetDevice();

        const D3D12_RESOURCE_DESC desc = m_finalBuffer->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
        UINT numRows = 0; // TODO: Remove
        UINT64 rowSizeInBytes = 0;
        UINT64 totalBytes = 0;

        device->GetCopyableFootprints(
            &desc,
            0, // FirstSubresource
            1, // NumSubresources
            0, // BaseOffset
            &footprint, // 出力: 各サブリソースの配置情報
            &numRows,
            &rowSizeInBytes,
            &totalBytes
        );

        // -----------------------------------------------
        // CPU --> uploadBuffer

        const uint8_t* src = static_cast<const uint8_t*>(image.getPointer());
        const size_t widthInBytes = image.size.x * image.pixelSizeInBytes();
        for (UINT y = 0; y < image.size.y; ++y)
        {
            memcpy(
                frameResource.dest + footprint.Offset + footprint.Footprint.RowPitch * y,
                src + y * widthInBytes,
                widthInBytes
            );
        }

        // -----------------------------------------------
        // uploadBuffer --> m_finalBuffer

        D3D12_TEXTURE_COPY_LOCATION dstCopyLocation{};
        dstCopyLocation.pResource = m_finalBuffer.Get();
        dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstCopyLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation{};
        srcCopyLocation.pResource = frameResource.uploadBuffer.Get();
        srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcCopyLocation.PlacedFootprint = footprint;

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Copy);

        commandList->CopyTextureRegion(&dstCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
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

    TextureResource DynamicTexture::getResource() const
    {
        return p_impl ? p_impl->m_textureResource : TextureResource{};
    }
}
