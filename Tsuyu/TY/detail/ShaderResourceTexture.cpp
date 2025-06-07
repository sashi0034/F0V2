#include "pch.h"
#include "ShaderResourceTexture.h"

#include "EngineCore.h"
#include "TY/AssertObject.h"
#include "TY/Color.h"
#include "TY/Image.h"
#include "TY/System.h"
#include "TY/TextureSource.h"
#include "TY/Utils.h"
#include "TY/Variant.h"

using namespace TY::detail;

struct ShaderResourceTexture::Impl
{
    ComPtr<ID3D12Resource> m_textureBuffer{};
    DXGI_FORMAT m_format{};
    Size m_size{};

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
        ID3D12Resource* uploadBuffer{};
        AssertWin32{"failed to create commited resource"sv}
            | EngineCore::GetDevice()->CreateCommittedResource(
                &uploadBufferDesc,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ, // CPUからの書き込みが可能な状態にする
                nullptr,
                IID_PPV_ARGS(&uploadBuffer));

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
            | EngineCore::GetDevice()->CreateCommittedResource(
                &textureHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, // コピー先として使用する
                nullptr,
                IID_PPV_ARGS(&m_textureBuffer)
            );

        // -----------------------------------------------
        // アップロード用中間バッファに生データをコピー
        uint8_t* imageMap{};
        AssertWin32{"failed to map upload buffer"sv}
            | uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&imageMap));

        uint8_t* srcAddress = rawImage->pixels;
        const auto srcPitch = AlignedSize(rawImage->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        for (size_t y = 0; y < rawImage->height; ++y)
        {
            std::memcpy(imageMap, srcAddress, rawImage->rowPitch);
            srcAddress += rawImage->rowPitch;
            imageMap += srcPitch;
        }

        uploadBuffer->Unmap(0, nullptr);

        // -----------------------------------------------
        // アップロード用中間バッファからテクスチャバッファにコピー
        // アップロード用中間バッファにはフットプリントを指定し、テクスチャバッファにはインデックスを指定する
        D3D12_TEXTURE_COPY_LOCATION dstCopyLocation{};
        dstCopyLocation.pResource = m_textureBuffer.Get();
        dstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstCopyLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcCopyLocation{};
        srcCopyLocation.pResource = uploadBuffer;
        srcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; // Footprint: メモリ占有領域に関する情報

        // FIXME: GetCopyableFootprints を使うほうがいい?
        srcCopyLocation.PlacedFootprint.Offset = 0;
        srcCopyLocation.PlacedFootprint.Footprint.Width = static_cast<UINT>(metadata.width);
        srcCopyLocation.PlacedFootprint.Footprint.Height = static_cast<UINT>(metadata.height);
        srcCopyLocation.PlacedFootprint.Footprint.Depth = static_cast<UINT>(metadata.depth);
        srcCopyLocation.PlacedFootprint.Footprint.RowPitch =
            static_cast<UINT>(AlignedSize(rawImage->rowPitch, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT));
        srcCopyLocation.PlacedFootprint.Footprint.Format = rawImage->format;

        const auto copyCommandList = EngineCore::GetCopyCommandList();

        copyCommandList->CopyTextureRegion(&dstCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);

        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_textureBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_COPY_SOURCE);
        copyCommandList->ResourceBarrier(1, &barrier);

        m_format = metadata.format;

        m_size = Size{static_cast<int>(metadata.width), static_cast<int>(metadata.height)};
    }

    Impl(const Image& image)
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
        resourceDesc.Width = image.size().x;
        resourceDesc.Height = image.size().y;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        AssertWin32{"failed to create commited resource"sv}
            | EngineCore::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_textureBuffer));

        AssertWin32{"failed to write to subresource"sv}
            | m_textureBuffer->WriteToSubresource(
                0,
                nullptr, // リソース全体領域をコピー
                image.data(),
                image.size().x * sizeof(ColorU8),
                image.size_in_bytes());

        m_format = resourceDesc.Format;

        m_size = Size{static_cast<int>(resourceDesc.Width), static_cast<int>(resourceDesc.Height)};
    }

    Impl(ID3D12Resource* resource)
    {
        m_textureBuffer = ComPtr<ID3D12Resource>(resource);
        m_format = resource->GetDesc().Format;
        m_size = Size{static_cast<int>(resource->GetDesc().Width), static_cast<int>(resource->GetDesc().Height)};
    }
};

namespace TY::detail
{
    ShaderResourceTexture::ShaderResourceTexture(const TextureSource& source)
    {
        if (const auto path = source.tryGet<std::string>())
        {
            p_impl = std::make_shared<Impl>(ToUtf16(*path));
        }
        else if (const auto image = source.tryGet<Image>())
        {
            p_impl = std::make_shared<Impl>(*image);
        }
        else if (const auto resource = source.tryGet<ID3D12Resource*>())
        {
            p_impl = std::make_shared<Impl>(*resource);
        }
        else
        {
            assert(false);
        }
    }

    bool ShaderResourceTexture::isEmpty() const
    {
        return p_impl == nullptr;
    }

    Size ShaderResourceTexture::size() const
    {
        return p_impl ? p_impl->m_size : Size{};
    }

    ID3D12Resource* ShaderResourceTexture::getResource() const
    {
        return p_impl ? p_impl->m_textureBuffer.Get() : nullptr;
    }

    DXGI_FORMAT ShaderResourceTexture::getFormat() const
    {
        return p_impl ? p_impl->m_format : DXGI_FORMAT_UNKNOWN;
    }
}
