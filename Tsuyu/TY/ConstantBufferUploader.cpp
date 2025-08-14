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

    ComPtr<ID3D12Resource> m_dstBuffer{};

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
        uint8_t* dst{};
    };

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};
    size_t m_lastUploadTimestamp{};

    Impl(uint32_t sizeInBytes, uint32_t count)
        : m_sizeInBytes(sizeInBytes),
          m_materialCount(count)
    {
        m_alignedSize = AlignedSize(sizeInBytes, 256);

        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize * count);

        if (const auto hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_dstBuffer));
            FAILED(hr))
        {
            LogError.writeln("ConstantBufferUploader: Failed to create m_dstBuffer.");
            return;
        }

        m_dstBuffer->SetName(L"ConstantBuffer::m_dstBuffer");

        m_valid = true;
    }

    ~Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (frameResource.uploadBuffer && frameResource.dst)
            {
                frameResource.uploadBuffer->Unmap(0, nullptr);
            }
        }
    }

    void Upload(const uint8_t* data, uint32_t count)
    {
        const size_t previousUploadTimestamp = m_lastUploadTimestamp;
        m_lastUploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_lastUploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (not frameResource.uploadBuffer)
        {
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize * m_materialCount);

            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&frameResource.uploadBuffer));
                FAILED(hr))
            {
                LogError.writeln("ConstantBufferUploader: Failed to create uploadBuffer.");
                return;
            }
        }

        if (not frameResource.dst)
        {
            if (const HRESULT hr = frameResource.uploadBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.dst));
                FAILED(hr))
            {
                LogError.writeln(std::format("ConstantBufferUploader: Failed to map resource for 0x{:016x}",
                                             reinterpret_cast<size_t>(data)));
                return;
            }

            frameResource.uploadBuffer->SetName(L"ConstantBuffer::uploadBuffer");
        }

        uint8_t* dst = frameResource.dst;
        uint32_t srcOffset{};
        for (int i = 0; i < count; ++i)
        {
            std::memcpy(dst, data + srcOffset, m_sizeInBytes);
            srcOffset += m_sizeInBytes;
            dst += m_alignedSize;
        }

        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Copy);
        // FIXME?
        const auto copyCommandList = EngineRenderContext::ActiveCommandList();

        copyCommandList->CopyBufferRegion(
            m_dstBuffer.Get(),
            0,
            frameResource.uploadBuffer.Get(),
            0,
            m_alignedSize * count);

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.uploadBuffer->Unmap(0, nullptr);
            frameResource.dst = nullptr;
        }
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
        return p_impl ? p_impl->m_dstBuffer->GetGPUVirtualAddress() : 0;
    }
}
