#include "pch.h"
#include "ConstantBuffer.h"

#include "Logger.h"
#include "Uncopyable.h"
#include "Utils.h"
#include "detail/RenderContext_singleton.h"
#include "detail/PlacedBufferAllocator.h"

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

            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(unitSize * maxCapacity);

            assert(m.uploadBuffer.isEmpty());
            if (const HRESULT hr = PlacedBufferAllocator_singleton::Upload().createResource(
                    resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, m.uploadBuffer);
                FAILED(hr))
            {
                LogError.writeln("ConstantBuffer: Failed to create uploadBuffer.");
                return;
            }

            m.uploadBuffer.getResource()->SetName(L"ConstantBuffer::uploadBuffer");
        }

        uint8_t* FetchMappedPointer()
        {
            if (not m.mappedPointer)
            {
                if (const HRESULT hr = m.uploadBuffer.getResource()->Map(0, nullptr, reinterpret_cast<void**>(&m.mappedPointer));
                    FAILED(hr))
                {
                    LogError.writeln(std::format("ConstantBuffer: Failed to map resource for 0x{:016x}",
                                                 reinterpret_cast<uint64_t>(m.uploadBuffer.getResource())));
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
            return m.uploadBuffer.getResource();
        }

        int MaxCapacity() const
        {
            return m.maxCapacity;
        }

        bool HasCapacity() const
        {
            return not m.uploadBuffer.isEmpty() && m.indexInFrame < m.maxCapacity;
        }

        void Unmap()
        {
            if (not m.uploadBuffer.isEmpty() && m.mappedPointer)
            {
                m.uploadBuffer.getResource()->Unmap(0, nullptr);
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
            PlacedBufferAllocation uploadBuffer{};
            uint8_t* mappedPointer{};
            uint64_t unitSize{};
            size_t timestamp{};
            int indexInFrame{};
            int maxCapacity{};
        } m{};

        void dispose()
        {
            Unmap();

            m.uploadBuffer = {};
        }
    };
}

struct ConstantBufferImpl::Impl
{
    bool m_valid{};

    uint32_t m_sizeInBytes;
    size_t m_alignedSize{};

    PlacedBufferAllocation m_bufferAllocation{};

    using frame_resources = FrameResource;

    std::array<frame_resources, RenderContext_singleton::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

    Impl(uint32_t sizeInBytes)
        : m_sizeInBytes(sizeInBytes)
    {
        m_alignedSize = AlignedSize(sizeInBytes, 256);

        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_alignedSize);

        if (const auto hr = PlacedBufferAllocator_singleton::Default().createResource(
                resourceDesc, D3D12_RESOURCE_STATE_COMMON, m_bufferAllocation);
            FAILED(hr))
        {
            LogError.writeln("ConstantBuffer: Failed to create m_finalBuffer.");
            return;
        }

        m_bufferAllocation.getResource()->SetName(L"ConstantBuffer::m_bufferAllocation");

        m_valid = true;
    }

    ~Impl()
    {
    }

    void Upload(const uint8_t* data)
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = RenderContext_singleton::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % RenderContext_singleton::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        frameResource.StepTimestamp(m_uploadTimestamp);

        if (not frameResource.HasCapacity())
        {
            const int nextCapacity = Max(1, frameResource.MaxCapacity() * 2);
            frameResource.Rebuild(m_alignedSize, nextCapacity);
            if (not frameResource.HasCapacity()) return;
        }

        uint8_t* dest = frameResource.FetchMappedPointer();
        if (not dest)
        {
            return;
        }

        const size_t uploadOffset = frameResource.MappedPointerOffset();
        dest += uploadOffset;

        frameResource.StepIndexInFrame();

        std::memcpy(dest, data, m_sizeInBytes);

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        RenderContext_singleton::TargetCommandList()->CopyBufferRegion(
            m_bufferAllocation.getResource(),
            0,
            frameResource.UploadBuffer(),
            uploadOffset,
            m_alignedSize);

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_GENERIC_READ);

        // if (previousUploadTimestamp == 0)
        // {
        //     // 初回実行時は即アンマップする
        //     frameResource.Unmap();
        // }
    }
};

namespace TY
{
    ConstantBufferImpl::ConstantBufferImpl(uint32_t sizeInBytes)
        : p_impl(std::make_shared<Impl>(sizeInBytes))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void ConstantBufferImpl::upload(const void* data) const
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(data));
    }

    bool ConstantBufferImpl::isEmpty() const
    {
        return not p_impl;
    }

    size_t ConstantBufferImpl::alignedSize() const
    {
        return p_impl ? p_impl->m_alignedSize : 0;
    }

    uint64_t ConstantBufferImpl::bufferLocation() const
    {
        return p_impl ? p_impl->m_bufferAllocation.getResource()->GetGPUVirtualAddress() : 0;
    }
}
