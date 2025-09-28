#include "pch.h"
#include "ConstantBuffer.h"

#include "Logger.h"
#include "Utils.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    class FrameResource
    {
    public:
        void StepTimestamp(size_t timestamp)
        {
            if (m_lastTimestamp != timestamp)
            {
                m_lastTimestamp = timestamp;
                m_indexInFrame = 0;
            }
        }

        bool HasCapacity() const
        {
            return m_indexInFrame < m_maxCapacity;
        }

        void Create(uint64_t unitSize, int maxCapacity)
        {
            m_unitSize = unitSize;
            m_maxCapacity = maxCapacity;

            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(unitSize * maxCapacity);

            assert(not m_uploadBuffer);
            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&m_uploadBuffer));
                FAILED(hr))
            {
                LogError.writeln("ConstantBuffer: Failed to create uploadBuffer.");
                return;
            }

            m_uploadBuffer->SetName(L"ConstantBuffer::uploadBuffer");
        }

        int Capacity() const
        {
            return m_maxCapacity;
        }

        ID3D12Resource* UploadBuffer() const
        {
            return m_uploadBuffer.Get();
        }

        uint8_t* TakeDest()
        {
            assert(HasCapacity());

            if (not m_dest)
            {
                if (const HRESULT hr = m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_dest));
                    FAILED(hr))
                {
                    LogError.writeln(std::format("ConstantBuffer: Failed to map resource for 0x{:016x}",
                                                 reinterpret_cast<uint64_t>(m_uploadBuffer.Get())));
                    return nullptr;
                }
            }

            uint8_t* dest = m_dest + m_unitSize * m_indexInFrame;
            ++m_indexInFrame;
            return dest;
        }

        void Unmap()
        {
            if (m_uploadBuffer && m_dest)
            {
                m_uploadBuffer->Unmap(0, nullptr);
                m_dest = nullptr;
            }
        }

        void Dispose()
        {
            Unmap();

            EngineRenderContext::SafeDisposeRenderResource(m_uploadBuffer);
        }

    private:
        ComPtr<ID3D12Resource> m_uploadBuffer{};
        uint8_t* m_dest{};
        uint64_t m_unitSize{};
        int m_indexInFrame{};
        int m_maxCapacity{};
        size_t m_lastTimestamp{};
    };
}

struct ConstantBufferCore::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    uint32_t m_materialCount;
    size_t m_alignedSize{};

    ComPtr<ID3D12Resource> m_finalBuffer{};

    using frame_resources = FrameResource;

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

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
                IID_PPV_ARGS(&m_finalBuffer));
            FAILED(hr))
        {
            LogError.writeln("ConstantBuffer: Failed to create m_finalBuffer.");
            return;
        }

        m_finalBuffer->SetName(L"ConstantBuffer::m_finalBuffer");

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_finalBuffer.Get(),
            D3D12_RESOURCE_STATE_COMMON,
            D3D12_RESOURCE_STATE_GENERIC_READ);
        commandList->ResourceBarrier(1, &barrier);

        m_valid = true;
    }

    ~Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            frameResource.Dispose();
        }

        EngineRenderContext::SafeDisposeRenderResource(m_finalBuffer);
    }

    void Upload(const uint8_t* data, uint32_t count)
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        frameResource.StepTimestamp(m_uploadTimestamp);

        if (not frameResource.HasCapacity())
        {
            frameResource.Dispose();
            const int nextCapacity = Max(1, frameResource.Capacity() * 2);
            frameResource = FrameResource{};
            frameResource.Create(m_alignedSize * m_materialCount, nextCapacity);
        }

        uint8_t* dest = frameResource.TakeDest();
        if (not dest)
        {
            return;
        }

        uint32_t srcOffset{};
        for (int i = 0; i < count; ++i)
        {
            std::memcpy(dest, data + srcOffset, m_sizeInBytes);
            srcOffset += m_sizeInBytes;
            dest += m_alignedSize;
        }

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Copy);
        commandList->CopyBufferRegion(
            m_finalBuffer.Get(),
            0,
            frameResource.UploadBuffer(),
            0,
            m_alignedSize * count);

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.Unmap();
        }
    }
};

namespace TY
{
    ConstantBufferCore::ConstantBufferCore(uint32_t sizeInBytes, uint32_t materialCount)
        : p_impl(std::make_shared<Impl>(sizeInBytes, materialCount))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool ConstantBufferCore::isEmpty() const
    {
        return not p_impl;
    }

    void ConstantBufferCore::upload(const void* data, uint32_t materialCount) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), materialCount);
    }

    uint32_t ConstantBufferCore::materialCount() const
    {
        return p_impl ? p_impl->m_materialCount : 0;
    }

    size_t ConstantBufferCore::sizeInBytes() const
    {
        return p_impl ? p_impl->m_sizeInBytes : 0;
    }

    size_t ConstantBufferCore::alignedSize() const
    {
        return p_impl ? p_impl->m_alignedSize : 0;
    }

    uint64_t ConstantBufferCore::bufferLocation() const
    {
        return p_impl ? p_impl->m_finalBuffer->GetGPUVirtualAddress() : 0;
    }
}
