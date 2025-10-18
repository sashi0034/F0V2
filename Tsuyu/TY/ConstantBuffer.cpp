#include "pch.h"
#include "ConstantBuffer.h"

#include "Logger.h"
#include "BufferHandle.h"
#include "Uncopyable.h"
#include "Utils.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    class FrameResource : Uncopyable
    {
    public:
        FrameResource() = default;

        void Rebuild(uint64_t unitSize, int maxCapacity)
        {
            const auto timestamp = m.timestamp;

            dispose();

            m = {};

            m.unitSize = unitSize;
            m.maxCapacity = maxCapacity;
            m.timestamp = timestamp; // 直前のタイムスタンプを引き継ぎ

            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(unitSize * maxCapacity);

            assert(not m.uploadBuffer);
            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(m.uploadBuffer.ReleaseAndGetAddressOf()));
                FAILED(hr))
            {
                LogError.writeln("ConstantBuffer: Failed to create uploadBuffer.");
                return;
            }

            m.uploadBuffer->SetName(L"ConstantBuffer::uploadBuffer");
        }

        uint8_t* FetchMappedPointer()
        {
            if (not m.mappedPointer)
            {
                if (const HRESULT hr = m.uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m.mappedPointer));
                    FAILED(hr))
                {
                    LogError.writeln(std::format("ConstantBuffer: Failed to map resource for 0x{:016x}",
                                                 reinterpret_cast<uint64_t>(m.uploadBuffer.Get())));
                    return nullptr;
                }
            }

            return m.mappedPointer;
        }

        size_t MappedPointerOffset() const
        {
            return m.unitSize * m.indexInFrame;
        }

        void StepTimestamp(size_t timestamp)
        {
            if (m.timestamp != timestamp)
            {
                m.timestamp = timestamp;
                m.indexInFrame = 0;
            }
        }

        void StepIndexInFrame()
        {
            assert(HasCapacity());
            ++m.indexInFrame;
        }

        ID3D12Resource* UploadBuffer() const
        {
            return m.uploadBuffer.Get();
        }

        int MaxCapacity() const
        {
            return m.maxCapacity;
        }

        bool HasCapacity() const
        {
            return m.indexInFrame < m.maxCapacity;
        }

        void Unmap()
        {
            if (m.uploadBuffer && m.mappedPointer)
            {
                m.uploadBuffer->Unmap(0, nullptr);
                m.mappedPointer = nullptr;
            }
        }

        ~FrameResource()
        {
            dispose();
        }

    private:
        struct member_t
        {
            ComPtr<ID3D12Resource> uploadBuffer{};
            uint8_t* mappedPointer{};
            uint64_t unitSize{};
            size_t timestamp{};
            int indexInFrame{};
            int maxCapacity{};
        } m{};

        void dispose()
        {
            Unmap();

            EngineRenderContext::SafeDisposeRenderResource(m.uploadBuffer);
        }
    };
}

struct ConstantBufferCore::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    uint32_t m_materialCount;
    size_t m_alignedSize{};

    BufferHandle m_bufferHandle{};

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
                IID_PPV_ARGS(m_bufferHandle.assignResourceAddress(D3D12_RESOURCE_STATE_COMMON)));
            FAILED(hr))
        {
            LogError.writeln("ConstantBuffer: Failed to create m_finalBuffer.");
            return;
        }

        m_bufferHandle.getResource()->SetName(L"ConstantBuffer::m_bufferHandle");

        m_valid = true;
    }

    ~Impl()
    {
    }

    void Upload(const uint8_t* data, uint32_t count, CommandListType commandListType)
    {
        assert(count <= m_materialCount);

        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        frameResource.StepTimestamp(m_uploadTimestamp);

        if (not frameResource.HasCapacity())
        {
            const int nextCapacity = Max(1, frameResource.MaxCapacity() * 2);
            frameResource.Rebuild(m_alignedSize * m_materialCount, nextCapacity);
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

        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        EngineRenderContext::TargetCommandList()->CopyBufferRegion(
            m_bufferHandle.getResource(),
            0,
            frameResource.UploadBuffer(),
            uploadOffset,
            m_alignedSize * count);

        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_GENERIC_READ);

        // if (previousUploadTimestamp == 0)
        // {
        //     // 初回実行時は即アンマップする
        //     frameResource.Unmap();
        // }
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
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data), materialCount, CommandListType::Draw);
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
        return p_impl ? p_impl->m_bufferHandle.getResource()->GetGPUVirtualAddress() : 0;
    }
}
