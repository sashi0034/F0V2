#include "pch.h"
#include "DiskTexture.h"

#include "AssertObject.h"
#include "System.h"
#include "Utils.h"
#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

struct DiskTexture::Impl
{
    DXGI_FORMAT m_format{};
    Size m_size{};

    TextureHandle m_textureHandle{};
    ComPtr<ID3D12Resource> m_uploadBuffer{};

    ~Impl()
    {
        RenderContext_singleton::SafeDisposeRenderResource(m_uploadBuffer);
    }

    Impl(const std::wstring& filename)
    {
        DirectX::TexMetadata metadata{};
        DirectX::ScratchImage scratchImage{};
        const auto loadResult =
            LoadFromWICFile(filename.c_str(), DirectX::WIC_FLAGS_NONE, &metadata, scratchImage);

        if (FAILED(loadResult))
        {
            System::ModalError(L"failed to load texture");
            throw std::runtime_error("failed to load texture"); // FIXME
        }

        // 生データを取得
        const auto rawImage = scratchImage.GetImage(0, 0, 0);

        // -----------------------------------------------
        // アップロード用中間バッファの設定
        D3D12_HEAP_PROPERTIES uploadBufferDesc{};
        uploadBufferDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
        uploadBufferDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        uploadBufferDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        uploadBufferDesc.CreationNodeMask = 0;
        uploadBufferDesc.VisibleNodeMask = 0;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = AlignedSize(rawImage->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT) * rawImage->height;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // アップロード用中間バッファの作成
        AssertWin32{"failed to create commited resource"sv}
            | RenderContext_singleton::GetDevice()->CreateCommittedResource(
                &uploadBufferDesc,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, // CPUからの書き込みが可能な状態にする
                nullptr,
                IID_PPV_ARGS(m_uploadBuffer.ReleaseAndGetAddressOf()));

        // -----------------------------------------------
        // テクスチャバッファ設定
        D3D12_HEAP_PROPERTIES textureHeapProperties{};
        textureHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        textureHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        textureHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        textureHeapProperties.CreationNodeMask = 0;
        textureHeapProperties.VisibleNodeMask = 0;

        resourceDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
        resourceDesc.Width = metadata.width;
        resourceDesc.Height = metadata.height;
        resourceDesc.DepthOrArraySize = metadata.arraySize;
        resourceDesc.MipLevels = metadata.mipLevels;
        resourceDesc.Format = metadata.format;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

        // テクスチャバッファ作成
        AssertWin32{""sv}
            | RenderContext_singleton::GetDevice()->CreateCommittedResource(
                &textureHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, // コピー先として使用する
                nullptr,
                IID_PPV_ARGS(m_textureHandle.assignResourceAddress(D3D12_RESOURCE_STATE_COPY_DEST))
            );

        // -----------------------------------------------
        // アップロード用中間バッファに生データをコピー
        uint8_t* imageMap{};
        AssertWin32{"failed to map upload buffer"sv}
            | m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&imageMap));

        uint8_t* srcAddress = rawImage->pixels;
        const auto srcPitch = AlignedSize(rawImage->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        for (size_t y = 0; y < rawImage->height; ++y)
        {
            std::memcpy(imageMap, srcAddress, rawImage->rowPitch);
            srcAddress += rawImage->rowPitch;
            imageMap += srcPitch;
        }

        m_uploadBuffer->Unmap(0, nullptr);

        // -----------------------------------------------
        // アップロード用中間バッファからテクスチャバッファにコピー
        // アップロード用中間バッファにはフットプリントを指定し、テクスチャバッファにはインデックスを指定する
        D3D12_TEXTURE_COPY_LOCATION dstCopyLocation{};
        dstCopyLocation.pResource = m_textureHandle.getResource();
        dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstCopyLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation{};
        srcCopyLocation.pResource = m_uploadBuffer.Get();
        srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // Footprint: メモリ占有領域に関する情報

        // FIXME: GetCopyableFootprints を使うほうがいい?
        srcCopyLocation.PlacedFootprint.Offset = 0;
        srcCopyLocation.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
        srcCopyLocation.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
        srcCopyLocation.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
        srcCopyLocation.PlacedFootprint.Footprint.RowPitch =
            static_cast<UINT>(AlignedSize(rawImage->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
        srcCopyLocation.PlacedFootprint.Footprint.Format = rawImage->format;

        const auto commandList = RenderContext_singleton::TargetCommandList();

        m_textureHandle.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        commandList->CopyTextureRegion(&dstCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);

        m_textureHandle.transitionResourceState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE); // FIXME?

        m_format = metadata.format;

        m_size = Size{static_cast<int>(metadata.width), static_cast<int>(metadata.height)};
    }
};

namespace TY
{
    DiskTexture::DiskTexture(const UnifiedString& path)
        : p_impl(std::make_shared<Impl>(path.toUtf16()))
    {
    }

    DiskTexture::operator TextureHandle() const
    {
        return p_impl ? p_impl->m_textureHandle : TextureHandle{};
    }
}
