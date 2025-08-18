#include "pch.h"
#include "StructuredBuffer.h"

#include "GpgpuBuffer.h"
#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct StructuredBuffer::Impl
{
    bool m_valid = false;

    UnorderedStructuredBufferParams m_params;
    bool m_writable{};

    ComPtr<ID3D12Resource> m_gpuBuffer;

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
        uint8_t* uploadDest{};

        ComPtr<ID3D12Resource> readbackBuffer;
        uint8_t* readbackSrc{};
    };

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};

    size_t m_flushTimestamp{};

    size_t m_dataSize{};

    Impl(const UnorderedStructuredBufferParams& params, bool isWritable) : m_params(params), m_writable(isWritable)
    {
        const auto device = EngineRenderContext::GetDevice();

        m_dataSize = params.elementCount * params.elementStride;
        if (m_dataSize <= 0)
        {
            LogError.writeln("StructuredBuffer: StructuredBuffer: Invalid data size.");
            return;
        }

        const D3D12_RESOURCE_FLAGS gpuBufferFlags =
            m_writable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;

        const CD3DX12_RESOURCE_DESC gpuBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_dataSize, gpuBufferFlags);

        CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};

        if (const auto hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &gpuBufferDesc,
                D3D12_RESOURCE_STATE_COMMON, // D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_PPV_ARGS(&m_gpuBuffer)
            );
            FAILED(hr))
        {
            LogError.writeln(std::format("StructuredBuffer: Failed to create GPU buffer: {}", hr));
            return;
        }

        m_valid = true;
    }

    void Upload(const uint8_t* src)
    {
        m_flushTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_flushTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (not ensureUploadBuffer(frameResource, m_dataSize)) return;

        memcpy(frameResource.uploadDest, src, m_dataSize);

        // GPU へアップロード
        const auto copyCommandList = EngineRenderContext::GetCommandList(CommandListType::Copy);;
        copyCommandList->CopyResource(m_gpuBuffer.Get(), frameResource.uploadBuffer.Get());

        // CopyResource で COPY_DEST 状態になっている m_gpuBuffer を、UNORDERED_ACCESS に移す
        if (m_writable)
        {
            const auto computeCommandList = EngineRenderContext::GetCommandList(CommandListType::Compute);;
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_gpuBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            computeCommandList->ResourceBarrier(1, &barrier);
        }
    }

    void AfterDispatch()
    {
        assert(m_writable);;

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuBuffer.Get());
        commandList->ResourceBarrier(1, &uavBarrier);
    }

    static void AfterDispatch(const Array<UnorderedStructuredBuffer>& list)
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);

        Array<D3D12_RESOURCE_BARRIER> barriers{};
        barriers.reserve(8);

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        for (int i = 0; i < list.size(); ++i)
        {
            auto& impl = list[i].p_impl;
            if (not impl) continue;

            const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(impl->m_gpuBuffer.Get());
            barriers.push_back(uavBarrier);
        }

        commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    }

    void BeforeFlush()
    {
        assert(m_writable);

        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuBuffer.Get());
        commandList->ResourceBarrier(1, &uavBarrier);

        // GPU バッファを COPY_SOURCE に遷移
        const auto toCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(
            m_gpuBuffer.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE);
        commandList->ResourceBarrier(1, &toCopySrc);

        // Copy GPU -> Readback
        m_flushTimestamp = EngineRenderContext::GetFlushTimestamp();
        const size_t frameIndex = m_flushTimestamp % EngineRenderContext::FrameBufferCount;
        auto& frameResource = m_frameResources[frameIndex];

        if (not ensureReadbackBuffer(frameResource, m_dataSize)) return;

        commandList->CopyResource(frameResource.readbackBuffer.Get(), m_gpuBuffer.Get());

        // GPU バッファを UNORDERED_ACCESS に戻す
        const auto toUAV = CD3DX12_RESOURCE_BARRIER::Transition(
            m_gpuBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &toUAV);
    }

    static void BeforeFlush(const Array<UnorderedStructuredBuffer>& list)
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Compute);

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        Array<D3D12_RESOURCE_BARRIER> barriers;
        barriers.reserve(list.size() * 2);

        // GPU バッファを COPY_SOURCE に遷移
        for (int i = 0; i < list.size(); ++i)
        {
            auto& impl = list[i].p_impl;
            if (not impl) continue;

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(impl->m_gpuBuffer.Get()));
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                    impl->m_gpuBuffer.Get(),
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                    D3D12_RESOURCE_STATE_COPY_SOURCE)
            );
        }

        commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

        // ReadbackBuffer へコピー
        for (int i = 0; i < list.size(); ++i)
        {
            auto& impl = list[i].p_impl;
            if (not impl) continue;

            impl->m_flushTimestamp = EngineRenderContext::GetFlushTimestamp();
            const size_t frameIndex = impl->m_flushTimestamp % EngineRenderContext::FrameBufferCount;
            auto& frameResource = impl->m_frameResources[frameIndex];

            if (not ensureReadbackBuffer(frameResource, impl->m_dataSize)) return;

            commandList->CopyResource(frameResource.readbackBuffer.Get(), impl->m_gpuBuffer.Get());
        }

        // GPU バッファを UNORDERED_ACCESS に戻す
        barriers.clear();
        for (int i = 0; i < list.size(); ++i)
        {
            auto& impl = list[i].p_impl;
            if (not impl) continue;

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                    impl->m_gpuBuffer.Get(),
                    D3D12_RESOURCE_STATE_COPY_SOURCE,
                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
            );
        }

        commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
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

        const size_t frameIndex = m_flushTimestamp % EngineRenderContext::FrameBufferCount;
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

            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
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
                LogError.writeln("StructuredBuffer: Failed to map resource.");
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
            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
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
                LogError.writeln("StructuredBuffer: Failed to map readback buffer.");
                return false;
            }
        }

        return true;
    }
};

namespace TY
{
    UnorderedStructuredBufferParams UnorderedStructuredBufferParams::From(
        const std::shared_ptr<detail::IGpgpuBuffer>& buffer)
    {
        if (not buffer)
        {
            return {};
        }

        UnorderedStructuredBufferParams params{
            .elementCount = buffer->getElementCount(),
            .elementStride = buffer->getElementStride()
        };

        return params;
    }

    StructuredBuffer::StructuredBuffer(const UnorderedStructuredBufferParams& params)
        : p_impl(std::make_shared<Impl>(params, false))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void StructuredBuffer::upload(const void* src)
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(src));
    }

    int StructuredBuffer::elementCount() const
    {
        return p_impl ? p_impl->m_params.elementCount : 0;
    }

    int StructuredBuffer::elementStride() const
    {
        return p_impl ? p_impl->m_params.elementStride : 0;
    }

    ID3D12Resource* StructuredBuffer::getBuffer() const
    {
        return p_impl ? p_impl->m_gpuBuffer.Get() : nullptr;
    }

    UnorderedStructuredBuffer::UnorderedStructuredBuffer(const UnorderedStructuredBufferParams& params)
    {
        p_impl = std::make_shared<Impl>(params, true);
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

    void UnorderedStructuredBuffer::AfterDispatch(const Array<UnorderedStructuredBuffer>& list)
    {
        Impl::AfterDispatch(list);
    }

    void UnorderedStructuredBuffer::beforeFlush()
    {
        if (p_impl)
        {
            p_impl->BeforeFlush();
        }
    }

    void UnorderedStructuredBuffer::BeforeFlush(const Array<UnorderedStructuredBuffer>& list)
    {
        Impl::BeforeFlush(list);
    }

    void UnorderedStructuredBuffer::readback(void* dst)
    {
        if (p_impl)
        {
            p_impl->Readback(static_cast<uint8_t*>(dst));
        }
    }
}
