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
        FrameResource() = default;

        FrameResource(uint64_t unitSize, int maxCapacity)
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

        uint8_t* FetchMappedPointer()
        {
            if (not m_mappedPointer)
            {
                if (const HRESULT hr = m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedPointer));
                    FAILED(hr))
                {
                    LogError.writeln(std::format("ConstantBuffer: Failed to map resource for 0x{:016x}",
                                                 reinterpret_cast<uint64_t>(m_uploadBuffer.Get())));
                    return nullptr;
                }
            }

            return m_mappedPointer;
        }

        size_t MappedPointerOffset() const
        {
            return m_unitSize * m_indexInFrame;
        }

        void StepTimestamp(size_t timestamp)
        {
            if (m_timestamp != timestamp)
            {
                m_timestamp = timestamp;
                m_indexInFrame = 0;
            }
        }

        void StepIndexInFrame()
        {
            assert(HasCapacity());
            ++m_indexInFrame;
        }

        ID3D12Resource* UploadBuffer() const
        {
            return m_uploadBuffer.Get();
        }

        int MaxCapacity() const
        {
            return m_maxCapacity;
        }

        bool HasCapacity() const
        {
            return m_indexInFrame < m_maxCapacity;
        }

        void Unmap()
        {
            if (m_uploadBuffer && m_mappedPointer)
            {
                m_uploadBuffer->Unmap(0, nullptr);
                m_mappedPointer = nullptr;
            }
        }

        void Dispose()
        {
            Unmap();

            EngineRenderContext::SafeDisposeRenderResource(m_uploadBuffer);
        }

    private:
        ComPtr<ID3D12Resource> m_uploadBuffer{};
        uint8_t* m_mappedPointer{};
        uint64_t m_unitSize{};
        size_t m_timestamp{};
        int m_indexInFrame{};
        int m_maxCapacity{};
    };
}

struct ConstantBufferCore::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    uint32_t m_materialCount;
    size_t m_alignedSize{};

    ComPtr<ID3D12Resource> m_finalBuffer{};

    D3D12_RESOURCE_STATES m_finalBufferState = D3D12_RESOURCE_STATE_COMMON;

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

    void Upload(const uint8_t* data, uint32_t count, CommandListType commandListType)
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        frameResource.StepTimestamp(m_uploadTimestamp);

        if (not frameResource.HasCapacity())
        {
            const int nextCapacity = Max(1, frameResource.MaxCapacity() * 2);
            frameResource.Dispose();
            frameResource = FrameResource{m_alignedSize * m_materialCount, nextCapacity};
        }

        uint8_t* dest = frameResource.FetchMappedPointer();
        if (not dest)
        {
            return;
        }

        const size_t uploadOffset = frameResource.MappedPointerOffset();
        dest += uploadOffset;

        frameResource.StepIndexInFrame();

        uint32_t srcOffset{};
        for (int i = 0; i < count; ++i)
        {
            std::memcpy(dest, data + srcOffset, m_sizeInBytes);
            srcOffset += m_sizeInBytes;
            dest += m_alignedSize;
        }

        const auto commandList = EngineRenderContext::GetCommandList(commandListType);

        changeFinalBufferState(commandList, D3D12_RESOURCE_STATE_COPY_DEST);

        commandList->CopyBufferRegion(
            m_finalBuffer.Get(),
            0,
            frameResource.UploadBuffer(),
            uploadOffset,
            m_alignedSize * count);

        if (commandListType == CommandListType::Draw)
        {
            changeFinalBufferState(commandList, D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.Unmap();
        }
    }

private:
    void changeFinalBufferState(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES newState)
    {
        if (m_finalBufferState == newState)
        {
            return;
        }

        const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_finalBuffer.Get(),
            m_finalBufferState,
            newState);
        commandList->ResourceBarrier(1, &barrier);

        m_finalBufferState = newState;
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
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), materialCount, CommandListType::Copy);
    }

    void ConstantBufferCore::uploadToDraw(const void* data, uint32_t materialCount) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), materialCount, CommandListType::Draw);
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
