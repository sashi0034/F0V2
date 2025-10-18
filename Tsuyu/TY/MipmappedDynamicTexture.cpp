#include "pch.h"
#include "MipmappedDynamicTexture.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct MipmappedDynamicTexture::Impl
{
    bool m_valid{};

    DXGI_FORMAT m_format{};
    Size m_size{};

    ComPtr<ID3D12Resource> m_uploadBuffer{};
    TextureHandle m_textureHandle{};

    // struct frame_resources
    // {
    //     ComPtr<ID3D12Resource> uploadBuffer;
    //     uint8_t* dest{};
    // };
    //
    // std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};
    //
    // size_t m_uploadTimestamp{};

    Impl(const ImageView& image)
    {
        m_format = image.format;

        m_size = image.size;

        if (not Create(image))
        {
            return;
        }

        m_valid = true;
    }

    ~Impl()
    {
        // for (auto& frameResource : m_frameResources)
        // {
        //     if (frameResource.uploadBuffer && frameResource.dest)
        //     {
        //         frameResource.uploadBuffer->Unmap(0, nullptr);
        //     }
        //
        //     EngineRenderContext::SafeDisposeRenderResource(frameResource.uploadBuffer);
        // }

        EngineRenderContext::SafeDisposeRenderResource(m_uploadBuffer); // TODO
    }

    bool Create(const ImageView& image)
    {
        ID3D12Device* device = EngineRenderContext::GetDevice();

        // -----------------------------------------------
        // ImageView --> ScratchImage

        DirectX::Image dxImage{};
        dxImage.width = image.size.x;
        dxImage.height = image.size.y;
        dxImage.format = image.format;
        dxImage.rowPitch = image.size.x * image.pixelSizeInBytes();
        dxImage.slicePitch = dxImage.rowPitch * image.size.y;
        dxImage.pixels = const_cast<uint8_t*>(static_cast<const uint8_t*>(image.getPointer()));

        DirectX::ScratchImage srcImage;
        if (HRESULT hr = srcImage.InitializeFromImage(dxImage);
            FAILED(hr))
        {
            LogError("MipmappedTexture: InitializeFromImage failed.");
            return false;
        }

        // -----------------------------------------------
        // mipmap 作成

        DirectX::ScratchImage mipChain;
        if (const HRESULT hr = GenerateMipMaps(
                srcImage.GetImages(),
                srcImage.GetImageCount(),
                srcImage.GetMetadata(),
                DirectX::TEX_FILTER_DEFAULT, // 品質は必要に応じて変更
                0, // 0: 可能な限りのミップ数
                mipChain);
            FAILED(hr))
        {
            LogError("MipmappedTexture: GenerateMipMaps failed.");
            return false;
        }

        DirectX::TexMetadata meta = mipChain.GetMetadata();

        // if (forceSRGB) {
        //     meta.format = ToSRGB(static_cast<DXGI_FORMAT>(meta.format));
        // }

        // -----------------------------------------------
        // m_finalBuffer 作成

        D3D12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            meta.format,
            static_cast<UINT64>(meta.width),
            static_cast<UINT>(meta.height),
            static_cast<UINT16>(meta.arraySize),
            static_cast<UINT16>(meta.mipLevels));
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES defaultHeap(D3D12_HEAP_TYPE_DEFAULT);

        if (const HRESULT hr = device->CreateCommittedResource(
                &defaultHeap,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_COPY_DEST, // 転送先
                nullptr,
                IID_PPV_ARGS(m_textureHandle.assignResourceAddress(D3D12_RESOURCE_STATE_COPY_DEST)));
            FAILED(hr))
        {
            LogError(std::format("MipmappedTexture: CreateCommittedResource failed: {}", static_cast<int>(hr)));
            return false;
        }

        m_textureHandle.getResource()->SetName(L"MipmappedTexture::Texture");

        // -----------------------------------------------
        // uploadBuffer --> m_finalBuffer

        const UINT subresourceCount = static_cast<UINT>(mipChain.GetImageCount());
        std::vector<D3D12_SUBRESOURCE_DATA> subresources(subresourceCount);

        const DirectX::Image* imgs = mipChain.GetImages();
        for (UINT i = 0; i < subresourceCount; ++i)
        {
            subresources[i].pData = imgs[i].pixels;
            subresources[i].RowPitch = imgs[i].rowPitch;
            subresources[i].SlicePitch = imgs[i].slicePitch;
        }

        const UINT64 uploadSize = GetRequiredIntermediateSize(
            m_textureHandle.getResource(), 0, subresourceCount);
        CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);

        if (const HRESULT hr = device->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(m_uploadBuffer.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError(std::format("MipmappedTexture: Create upload buffer failed: {}", static_cast<int>(hr)));
            return false;
        }

        m_uploadBuffer->SetName(L"MipmappedTexture::Upload");

        // エンジンのコマンドリストに記録（実行・フェンス待ちはエンジン側ポリシーに従う）
        auto cmdList = EngineRenderContext::GetCommandList(CommandListType::Draw);

        UpdateSubresources(cmdList, m_textureHandle.getResource(), m_uploadBuffer.Get(), 0, 0, subresourceCount,
                           subresources.data());

        // 転送後：Pixel Shader から参照できる状態へ
        m_textureHandle.transitionResourceState(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        return true;
    }
};

namespace TY
{
    MipmappedDynamicTexture::MipmappedDynamicTexture(const ImageView& image)
        : p_impl{std::make_shared<Impl>(image)}
    {
        if (not p_impl->m_valid)
        {
            p_impl = nullptr;
        }
    }

    MipmappedDynamicTexture::operator TextureHandle() const
    {
        return p_impl ? p_impl->m_textureHandle : TextureHandle{};
    }
}
