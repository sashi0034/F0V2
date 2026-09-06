#include "pch.h"
#include "VertexBuffer.h"

#include "Logger.h"
#include "detail/RenderContext_singleton.h"
#include "detail/PlacedBufferAllocator.h"

using namespace TY;
using namespace TY::detail;

struct VertexBufferImpl::Impl
{
    bool m_valid{};

    D3D12_VERTEX_BUFFER_VIEW m_vertBufferView{};

    PlacedBufferAllocation m_bufferAllocation{};

    int m_count{};

    struct frame_resources
    {
        PlacedBufferAllocation uploadBuffer;
        uint8_t* dest{};
    };

    std::array<frame_resources, RenderContext_singleton::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

    Impl(int sizeInBytes, int strideInBytes)
    {
        const D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

        // リソース作成
        if (const auto hr = PlacedBufferAllocator_singleton::Default().createResource(
                resourceDesc, D3D12_RESOURCE_STATE_COMMON, m_bufferAllocation);
            FAILED(hr))
        {
            LogError.writeln("VertexBuffer: Failed to create m_bufferAllocation");
            return;
        }

        m_bufferAllocation.getResource()->SetName(L"VertexBuffer::m_bufferAllocation");

        m_vertBufferView.BufferLocation = m_bufferAllocation.getResource()->GetGPUVirtualAddress();
        m_vertBufferView.SizeInBytes = sizeInBytes;
        m_vertBufferView.StrideInBytes = strideInBytes;

        m_count = sizeInBytes / strideInBytes;

        m_valid = true;
    }

    ~Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (not frameResource.uploadBuffer.isEmpty() && frameResource.dest)
            {
                frameResource.uploadBuffer.getResource()->Unmap(0, nullptr);
            }
        }
    }

    void Upload(const void* data, size_t size)
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = RenderContext_singleton::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % RenderContext_singleton::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (frameResource.uploadBuffer.isEmpty())
        {
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertBufferView.SizeInBytes);

            if (const HRESULT hr = PlacedBufferAllocator_singleton::Upload().createResource(
                    resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, frameResource.uploadBuffer);
                FAILED(hr))
            {
                LogError.writeln("VertexBuffer: Failed to create uploadBuffer.");
                return;
            }

            frameResource.uploadBuffer.getResource()->SetName(L"VertexBuffer::uploadBuffer");
        }

        if (not frameResource.dest)
        {
            if (const HRESULT hr = frameResource.uploadBuffer.getResource()->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.dest));
                FAILED(hr))
            {
                LogError.writeln("VertexBuffer: Failed to map uploadBuffer");
                return;
            }
        }

        uint8_t* dest = frameResource.dest;
        memcpy(dest, data, size);

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        RenderContext_singleton::TargetCommandList()->CopyBufferRegion(
            m_bufferAllocation.getResource(),
            0,
            frameResource.uploadBuffer.getResource(),
            0,
            size);

        m_bufferAllocation.transitionResourceState(D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.uploadBuffer.getResource()->Unmap(0, nullptr);
            frameResource.dest = nullptr;
        }
    }

    void CommandSet() const
    {
        const auto commandList = RenderContext_singleton::TargetCommandList();
        commandList->IASetVertexBuffers(0, 1, &m_vertBufferView);
    }
};

namespace TY
{
    VertexBufferImpl::VertexBufferImpl(int sizeInBytes, int strideInBytes) :
        p_impl(std::make_shared<Impl>(sizeInBytes, strideInBytes))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool VertexBufferImpl::isEmpty() const
    {
        return p_impl == nullptr;
    }

    int VertexBufferImpl::count() const
    {
        return p_impl ? p_impl->m_count : 0;
    }

    size_t VertexBufferImpl::size_in_bytes() const
    {
        return p_impl ? p_impl->m_vertBufferView.SizeInBytes : 0;
    }

    void VertexBufferImpl::upload(const void* data)
    {
        if (not p_impl) return;
        p_impl->Upload(data, p_impl->m_vertBufferView.SizeInBytes);
    }

    void VertexBufferImpl::upload(const void* data, int count)
    {
        if (not p_impl) return;
        p_impl->Upload(data, p_impl->m_vertBufferView.StrideInBytes * count);
    }

    void VertexBufferImpl::commandSet() const
    {
        if (not p_impl) return;
        p_impl->CommandSet();
    }
}
