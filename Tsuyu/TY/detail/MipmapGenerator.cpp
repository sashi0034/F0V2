#include "pch.h"
#include "MipmapGenerator.h"

#include "ComputePipelineState.h"
#include "EnginePresetAsset.h"
#include "RenderContext_singleton.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

struct MipmapGenerator::Impl
{
    bool m_valid{};

    TextureHandle m_texture{};

    // [SRV (mip: i), UAV (mip: i + 1)] を mipCount - 1 組並べたヒープ
    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap{};

    Impl(const TextureHandle& texture)
        : m_texture(texture)
    {
        const auto resource = texture.getResource();
        if (not resource)
        {
            LogError("MipmapGenerator: Texture is empty.");
            return;
        }

        const auto desc = resource->GetDesc();
        if (not(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        {
            LogError("MipmapGenerator: Texture is not created with D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS.");
            return;
        }

        const int mipCount = texture.mipCount();
        if (mipCount <= 1)
        {
            return;
        }

        const auto device = RenderContext_singleton::GetDevice();

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = static_cast<UINT>((mipCount - 1) * 2);
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        if (const HRESULT hr = device->CreateDescriptorHeap(
                &heapDesc, IID_PPV_ARGS(m_descriptorHeap.ReleaseAndGetAddressOf()));
            FAILED(hr))
        {
            LogError(std::format("MipmapGenerator: Failed to create descriptor heap: {}", hr));
            return;
        }

        m_descriptorHeap->SetName(L"MipmapGenerator::m_descriptorHeap");

        const auto incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        auto heapHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        for (int i = 0; i < mipCount - 1; ++i)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = desc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = static_cast<UINT>(i);
            srvDesc.Texture2D.MipLevels = 1;
            device->CreateShaderResourceView(resource, &srvDesc, heapHandle);
            heapHandle.ptr += incrementSize;

            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
            uavDesc.Format = desc.Format;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = static_cast<UINT>(i + 1);
            device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, heapHandle);
            heapHandle.ptr += incrementSize;
        }

        m_valid = true;
    }

    ~Impl()
    {
        RenderContext_singleton::SafeDisposeRenderObject(m_descriptorHeap);
    }

    void Generate() const
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();
        const auto resource = m_texture.getResource();
        const auto desc = resource->GetDesc();
        const int mipCount = static_cast<int>(desc.MipLevels);

        // 全体を読み取り状態に揃え、書き込むミップだけ UAV にする
        const auto previousState = m_texture.getResourceState();
        constexpr auto readState = D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        m_texture.transitionResourceState(readState);

        EnginePresetAsset::GetGenerateMipsPSO().commandSet(CommandListType::Draw);
        commandList->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());

        const auto incrementSize = RenderContext_singleton::GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        for (int i = 0; i < mipCount - 1; ++i)
        {
            const UINT dstMip = static_cast<UINT>(i + 1);
            const UINT dstWidth = std::max<UINT>(1, static_cast<UINT>(desc.Width >> dstMip));
            const UINT dstHeight = std::max<UINT>(1, static_cast<UINT>(desc.Height >> dstMip));

            auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource, readState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, dstMip);
            commandList->ResourceBarrier(1, &barrier);

            auto tableHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
            tableHandle.ptr += static_cast<UINT64>(incrementSize) * i * 2;
            commandList->SetComputeRootDescriptorTable(0, tableHandle);

            commandList->Dispatch((dstWidth + 7) / 8, (dstHeight + 7) / 8, 1);

            barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                resource, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, readState, dstMip);
            commandList->ResourceBarrier(1, &barrier);
        }

        m_texture.transitionResourceState(previousState);
    }
};

namespace TY::detail
{
    MipmapGenerator::MipmapGenerator(const TextureHandle& texture)
        : p_impl(std::make_shared<Impl>(texture))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool MipmapGenerator::isEmpty() const
    {
        return p_impl == nullptr;
    }

    void MipmapGenerator::generate() const
    {
        if (p_impl) p_impl->Generate();
    }
}
