#include "pch.h"
#include "ConstantBufferUploader.h"

#include "Logger.h"
#include "System.h"
#include "Utils.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
}

struct ConstantBufferUploaderCore::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    uint32_t m_materialCount;
    size_t m_alignedSize{};

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
    };

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};
    ComPtr<ID3D12Resource> m_gpuBuffer{};

    Impl(uint32_t sizeInBytes, uint32_t count)
        : m_sizeInBytes(sizeInBytes),
          m_materialCount(count)
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        m_alignedSize = AlignedSize(sizeInBytes, 256);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize * count);

        if (const auto hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_gpuBuffer));
            FAILED(hr))
        {
            LogError.writeln("ConstantBufferUploader: Failed to create m_gpuBuffer.");
            return;
        }

        m_gpuBuffer->SetName(L"ConstantBuffer::m_gpuBuffer");

        // -----------------------------------------------

        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

        for (int i = 0; i < EngineRenderContext::FrameBufferCount; ++i)
        {
            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_frameResources[i].uploadBuffer));
                FAILED(hr))
            {
                LogError.writeln("ConstantBufferUploader: Failed to create uploadBuffer.");
                return;
            }
        }

        m_valid = true;
    }

    ~Impl()
    {
    }

    void Upload(const uint8_t* data, uint32_t count)
    {
        uint8_t* dest;

        // TODO: 永続マッピング

        size_t frameIndex = System::FrameCount() % EngineRenderContext::FrameBufferCount;
        if (const auto hr = m_frameResources[frameIndex].uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dest));
            FAILED(hr))
        {
            LogError.writeln(std::format("ConstantBufferUploader: Failed to map resource for 0x{:016x}",
                                         reinterpret_cast<size_t>(data)));
            return;
        }

        uint32_t srcOffset{};
        for (int i = 0; i < count; ++i)
        {
            std::memcpy(dest, data + srcOffset, m_sizeInBytes);
            srcOffset += m_sizeInBytes;
            dest += m_alignedSize;
        }

        m_frameResources[frameIndex].uploadBuffer->Unmap(0, nullptr);

        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Copy); // FIXME?
        const auto copyCommandList = EngineRenderContext::ActiveCommandList();

        copyCommandList->CopyResource(m_gpuBuffer.Get(), m_frameResources[frameIndex].uploadBuffer.Get());
        // TODO: CopyBufferRegion 
    }
};

namespace TY
{
    ConstantBufferUploaderCore::ConstantBufferUploaderCore(uint32_t sizeInBytes, uint32_t materialCount)
        : p_impl(std::make_shared<Impl>(sizeInBytes, materialCount))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool ConstantBufferUploaderCore::isEmpty() const
    {
        return not p_impl;
    }

    void ConstantBufferUploaderCore::upload(const void* data, uint32_t materialCount) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), materialCount);
    }

    uint32_t ConstantBufferUploaderCore::materialCount() const
    {
        return p_impl ? p_impl->m_materialCount : 0;
    }

    size_t ConstantBufferUploaderCore::sizeInBytes() const
    {
        return p_impl ? p_impl->m_sizeInBytes : 0;
    }

    size_t ConstantBufferUploaderCore::alignedSize() const
    {
        return p_impl ? p_impl->m_alignedSize : 0;
    }

    uint64_t ConstantBufferUploaderCore::bufferLocation() const
    {
        return p_impl ? p_impl->m_gpuBuffer->GetGPUVirtualAddress() : 0;
    }
}
