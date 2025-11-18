#include "pch.h"
#include "StructuredBuffer.h"

#include "BufferHandle.h"
#include "GpgpuBuffer.h"
#include "Logger.h"
#include "detail/RenderContext_singleton.h"

using namespace TY;
using namespace TY::detail;

// TODO: 昔の実装を書き直す 
struct StructuredBuffer::Impl
{
    bool m_valid = false;

    int m_elementCount = 0;
    int m_elementStride = 0;

    bool m_writable{};

    BufferHandle m_bufferHandle;

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
        uint8_t* uploadDest{};

        ComPtr<ID3D12Resource> readbackBuffer;
        uint8_t* readbackSrc{};
    };

    std::array<frame_resources, RenderContext_singleton::FrameBufferCount> m_frameResources{};

    size_t m_flushTimestamp{};

    size_t m_dataSize{};

    Impl(int elementCount, int elementStride, bool isWritable) :
        m_elementCount(elementCount),
        m_elementStride(elementStride),
        m_writable(isWritable)
    {
        const auto device = RenderContext_singleton::GetDevice();

        m_dataSize = elementCount * elementStride;
        if (m_dataSize <= 0)
        {
            LogError.writeln("StructuredBuffer: StructuredBuffer: Invalid data size.");
            return;
        }

        const D3D12_RESOURCE_FLAGS finalBufferFlags =
            m_writable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

        const CD3DX12_RESOURCE_DESC finalBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_dataSize, finalBufferFlags);

        CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};

        if (const auto hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &finalBufferDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(m_bufferHandle.assignResourceAddress(D3D12_RESOURCE_STATE_COMMON))
            );
            FAILED(hr))
        {
            LogError.writeln(std::format("StructuredBuffer: Failed to create GPU buffer: {}", hr));
            return;
        }

        m_valid = true;
    }

    Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (frameResource.uploadBuffer && frameResource.uploadDest)
            {
                frameResource.uploadBuffer->Unmap(0, nullptr);
            }

            if (frameResource.readbackBuffer && frameResource.readbackSrc)
            {
                frameResource.readbackBuffer->Unmap(0, nullptr);
            }

            RenderContext_singleton::SafeDisposeRenderResource(frameResource.uploadBuffer);
            RenderContext_singleton::SafeDisposeRenderResource(frameResource.readbackBuffer);
        }
    }

    void Upload(const uint8_t* src)
    {
        m_flushTimestamp = RenderContext_singleton::GetFlushTimestamp();

        const size_t frameIndex = m_flushTimestamp % RenderContext_singleton::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (not ensureUploadBuffer(frameResource, m_dataSize)) return;

        memcpy(frameResource.uploadDest, src, m_dataSize);

        // GPU へアップロード
        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_COPY_DEST);

        const auto commandList = RenderContext_singleton::TargetCommandList();;
        commandList->CopyResource(m_bufferHandle.getResource(), frameResource.uploadBuffer.Get());

        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        // TODO
        // m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    // TODO: 昔の実装を書き直す 
    void AfterDispatch()
    {
        assert(m_writable);;

        const auto commandList = RenderContext_singleton::TargetCommandList();

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bufferHandle.getResource());
        commandList->ResourceBarrier(1, &uavBarrier);
    }

    void BeforeFlush()
    {
        assert(m_writable);

        const auto commandList = RenderContext_singleton::TargetCommandList();

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_bufferHandle.getResource());
        commandList->ResourceBarrier(1, &uavBarrier);

        // GPU バッファを COPY_SOURCE に遷移
        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_COPY_SOURCE);

        // Copy GPU -> Readback
        m_flushTimestamp = RenderContext_singleton::GetFlushTimestamp();
        const size_t frameIndex = m_flushTimestamp % RenderContext_singleton::FrameBufferCount;
        auto& frameResource = m_frameResources[frameIndex];

        if (not ensureReadbackBuffer(frameResource, m_dataSize)) return;

        commandList->CopyResource(frameResource.readbackBuffer.Get(), m_bufferHandle.getResource());

        // GPU バッファを UNORDERED_ACCESS に戻す
        m_bufferHandle.transitionResourceState(D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // FIXME?
    }

    void Readback(uint8_t* dest)
    {
        // TODO: 複数バリアの最適化
        assert(m_writable);

        if (not dest)
        {
            LogError.writeln("UnorderedStructuredBuffer::Readback(): Destination pointer is null.");
            return;
        }

        const size_t frameIndex = m_flushTimestamp % RenderContext_singleton::FrameBufferCount;
        auto& frameResource = m_frameResources[frameIndex];
        assert(frameResource.readbackSrc);

        memcpy(dest, frameResource.readbackSrc, m_dataSize);
    }

private:
    static bool ensureUploadBuffer(frame_resources& frameResource, size_t dataSize)
    {
        if (not frameResource.uploadBuffer)
        {
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);

            if (const HRESULT hr = RenderContext_singleton::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&frameResource.uploadBuffer));
                FAILED(hr))
            {
                LogError.writeln("StructuredBuffer: Failed to create uploadBuffer.");
                return false;
            }

            frameResource.uploadBuffer->SetName(L"StructuredBuffer::uploadBuffer");
        }

        if (not frameResource.uploadDest)
        {
            if (const HRESULT hr = frameResource.uploadBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.uploadDest));
                FAILED(hr))
            {
                LogError.writeln("StructuredBuffer: Failed to map uploadBuffer.");
                return false;
            }
        }

        return true;
    }

    static bool ensureReadbackBuffer(frame_resources& frameResource, size_t dataSize)
    {
        if (not frameResource.readbackBuffer)
        {
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(dataSize);
            if (const HRESULT hr = RenderContext_singleton::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_COPY_DEST,
                    nullptr,
                    IID_PPV_ARGS(&frameResource.readbackBuffer));
                FAILED(hr))
            {
                LogError.writeln(std::format("StructuredBuffer: Failed to create readback buffer: {}", hr));
                return false;
            }

            frameResource.readbackBuffer->SetName(L"StructuredBuffer::readbackBuffer");
        }

        if (not frameResource.readbackSrc)
        {
            if (const HRESULT hr = frameResource.readbackBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.readbackSrc));
                FAILED(hr))
            {
                LogError.writeln("StructuredBuffer: Failed to map readbackBuffer.");
                return false;
            }
        }

        return true;
    }
};

namespace TY
{
    StructuredBuffer::StructuredBuffer(int elementCount, int elementStride)
        : p_impl(std::make_shared<Impl>(elementCount, elementStride, false))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool StructuredBuffer::isEmpty() const
    {
        return not p_impl;
    }

    void StructuredBuffer::upload(const void* src)
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(src));
    }

    int StructuredBuffer::elementCount() const
    {
        return p_impl ? p_impl->m_elementCount : 0;
    }

    int StructuredBuffer::elementStride() const
    {
        return p_impl ? p_impl->m_elementStride : 0;
    }

    ID3D12Resource* StructuredBuffer::getBuffer() const
    {
        return p_impl ? p_impl->m_bufferHandle.getResource() : nullptr;
    }

    UnorderedStructuredBuffer::UnorderedStructuredBuffer(int elementCount, int elementStride)
    {
        p_impl = std::make_shared<Impl>(elementCount, elementStride, true);
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void UnorderedStructuredBuffer::afterDispatch()
    {
        if (p_impl)
        {
            p_impl->AfterDispatch();
        }
    }

    void UnorderedStructuredBuffer::beforeFlush()
    {
        if (p_impl)
        {
            p_impl->BeforeFlush();
        }
    }

    void UnorderedStructuredBuffer::readback(void* dst)
    {
        if (p_impl)
        {
            p_impl->Readback(static_cast<uint8_t*>(dst));
        }
    }
}
