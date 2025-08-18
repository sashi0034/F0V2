#include "pch.h"
#include "StructuredBufferUploader.h"

#include "GpgpuBuffer.h"
#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct StructuredBufferUploader::Impl
{
    bool m_valid = false;

    StructuredBufferTransferParams m_params;
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

    Impl(const StructuredBufferTransferParams& params, bool isWritable) : m_params(params), m_writable(isWritable)
    {
        const auto device = EngineRenderContext::GetDevice();

        m_dataSize = params.elementCount * params.elementStride;
        if (m_dataSize <= 0)
        {
            LogError.writeln("StructuredBufferUploader: StructuredBufferUploader: Invalid data size.");
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
            LogError.writeln(std::format("StructuredBufferUploader: Failed to create GPU buffer: {}", hr));
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
        const auto commandTargetLifetime = EngineRenderContext::ScopedCommandTarget(CommandListType::Copy);
        const auto commandList = EngineRenderContext::ActiveCommandList();
        commandList->CopyResource(m_gpuBuffer.Get(), frameResource.uploadBuffer.Get());

        // CopyResource で COPY_DEST 状態になっている m_gpuBuffer を、UNORDERED_ACCESS に移す
        if (m_writable)
        {
            const auto computeCommandTargetLifetime = // FIXME: Simplify
                EngineRenderContext::ScopedCommandTarget(CommandListType::Compute);
            const auto computeCommandList = EngineRenderContext::ActiveCommandList();
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

        assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
        const auto commandList = EngineRenderContext::ActiveCommandList();

        // UAV バリアを入れて、UAV 書き込みの完了を保証
        const D3D12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuBuffer.Get());
        commandList->ResourceBarrier(1, &uavBarrier);
    }

    static void AfterDispatch(const Array<StructuredBufferTransfer>& list)
    {
        assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
        const auto commandList = EngineRenderContext::ActiveCommandList();

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

        assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
        const auto commandList = EngineRenderContext::ActiveCommandList();

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

    static void BeforeFlush(const Array<StructuredBufferTransfer>& list)
    {
        assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
        const auto commandList = EngineRenderContext::ActiveCommandList();

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
            LogError.writeln("StructuredBufferTransfer::Readback(): Destination pointer is null.");
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
                LogError.writeln("StructuredBufferUploader: Failed to create uploadBuffer.");
                return false;
            }

            frameResource.uploadBuffer->SetName(L"StructuredBufferUploader::uploadBuffer");
        }

        if (not frameResource.uploadDest)
        {
            if (const HRESULT hr = frameResource.uploadBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.uploadDest));
                FAILED(hr))
            {
                LogError.writeln("StructuredBufferUploader: Failed to map resource.");
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
                LogError.writeln(std::format("StructuredBufferUploader: Failed to create readback buffer: {}", hr));
                return false;
            }

            frameResource.readbackBuffer->SetName(L"StructuredBufferUploader::readbackBuffer");
        }

        if (not frameResource.readbackSrc)
        {
            if (const HRESULT hr = frameResource.readbackBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.readbackSrc));
                FAILED(hr))
            {
                LogError.writeln("StructuredBufferUploader: Failed to map readback buffer.");
                return false;
            }
        }

        return true;
    }
};

namespace TY
{
    StructuredBufferTransferParams StructuredBufferTransferParams::From(
        const std::shared_ptr<detail::IGpgpuBuffer>& buffer)
    {
        if (not buffer)
        {
            return {};
        }

        StructuredBufferTransferParams params{
            .elementCount = buffer->getElementCount(),
            .elementStride = buffer->getElementStride()
        };

        return params;
    }

    StructuredBufferUploader::StructuredBufferUploader(const StructuredBufferTransferParams& params)
        : p_impl(std::make_shared<Impl>(params, false))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void StructuredBufferUploader::upload(const void* src)
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(src));
    }

    int StructuredBufferUploader::elementCount() const
    {
        return p_impl ? p_impl->m_params.elementCount : 0;
    }

    int StructuredBufferUploader::elementStride() const
    {
        return p_impl ? p_impl->m_params.elementStride : 0;
    }

    ID3D12Resource* StructuredBufferUploader::getBuffer() const
    {
        return p_impl ? p_impl->m_gpuBuffer.Get() : nullptr;
    }

    StructuredBufferTransfer::StructuredBufferTransfer(const StructuredBufferTransferParams& params)
    {
        p_impl = std::make_shared<Impl>(params, true);
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void StructuredBufferTransfer::afterDispatch()
    {
        if (p_impl)
        {
            p_impl->AfterDispatch();
        }
    }

    void StructuredBufferTransfer::AfterDispatch(const Array<StructuredBufferTransfer>& list)
    {
        Impl::AfterDispatch(list);
    }

    void StructuredBufferTransfer::beforeFlush()
    {
        if (p_impl)
        {
            p_impl->BeforeFlush();
        }
    }

    void StructuredBufferTransfer::BeforeFlush(const Array<StructuredBufferTransfer>& list)
    {
        Impl::BeforeFlush(list);
    }

    void StructuredBufferTransfer::readback(void* dst)
    {
        if (p_impl)
        {
            p_impl->Readback(static_cast<uint8_t*>(dst));
        }
    }
}
