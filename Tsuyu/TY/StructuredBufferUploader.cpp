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
    ComPtr<ID3D12Resource> m_uploadBuffer; // TODO: IB, VB, CB などと同様にダブルバッファリングに修正
    ComPtr<ID3D12Resource> m_readbackBuffer;
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

        // m_uploadBuffer を作成
        const CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            m_dataSize,
            D3D12_RESOURCE_FLAG_NONE
        );

        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        if (const HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_uploadBuffer));
            FAILED(hr))
        {
            LogError.writeln(std::format("StructuredBufferUploader: Failed to create upload buffer: {}", hr));
            return;
        }

        // m_readbackBuffer を作成
        const CD3DX12_HEAP_PROPERTIES readbackHeapProps(D3D12_HEAP_TYPE_READBACK);
        const CD3DX12_RESOURCE_DESC readbackBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_dataSize);

        if (const HRESULT hr = device->CreateCommittedResource(
                &readbackHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &readbackBufferDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_readbackBuffer));
            FAILED(hr))
        {
            LogError.writeln(std::format("StructuredBufferUploader: Failed to create readback buffer: {}", hr));
            return;
        }

        m_valid = true;
    }

    void Upload(const uint8_t* src)
    {
        // マップして書き込み
        uint8_t* dest;

        if (const HRESULT hr = m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dest));
            FAILED(hr))
        {
            LogError.writeln(std::format("StructuredBufferUploader::Upload(): Failed to map upload buffer: {}", hr));
            return;
        }

        memcpy(dest, src, m_dataSize);

        m_uploadBuffer->Unmap(0, nullptr);

        // GPU へアップロード
        assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
        const auto commandList = EngineRenderContext::ActiveCommandList();
        commandList->CopyResource(m_gpuBuffer.Get(), m_uploadBuffer.Get());

        // CopyResource で COPY_DEST 状態になっている m_gpuBuffer を、UNORDERED_ACCESS に移す
        if (m_writable)
        {
            const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                m_gpuBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            commandList->ResourceBarrier(1, &barrier);
        }
    }

    // static void Upload(const Array<StructuredBufferUploader>& list, const Array<uint8_t*>& srcs)
    // {
    //     assert(EngineRenderContext::ActiveCommandTarget() == CommandListType::Compute);
    //     const auto commandList = EngineRenderContext::ActiveCommandList();
    //
    //     for (int i = 0; i < list.size(); ++i)
    //     {
    //         auto& impl = list[i].p_impl;
    //         if (not impl) continue;
    //
    //         auto& src = srcs[i];
    //
    //         // マップして書き込み
    //         uint8_t* dest;
    //         if (FAILED(impl->m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dest))))
    //         {
    //             LogError.writeln("StructuredBufferUploader::Upload(): Failed to map upload buffer.");
    //             continue;
    //         }
    //
    //         memcpy(dest, src, impl->m_dataSize);
    //         impl->m_uploadBuffer->Unmap(0, nullptr);
    //
    //         // GPU へアップロード
    //         commandList->CopyResource(impl->m_gpuBuffer.Get(), impl->m_uploadBuffer.Get());
    //     }
    //
    //     Array<D3D12_RESOURCE_BARRIER> barriers;
    //     barriers.reserve(list.size());
    //
    //     // CopyResource で COPY_DEST 状態になっている m_gpuBuffer を、UNORDERED_ACCESS に移す
    //     for (int i = 0; i < list.size(); ++i)
    //     {
    //         auto& impl = list[i].p_impl;
    //         if (not impl) continue;
    //
    //         if (impl->m_writable)
    //         {
    //             barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
    //                 impl->m_gpuBuffer.Get(),
    //                 D3D12_RESOURCE_STATE_COPY_DEST,
    //                 D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
    //         }
    //     }
    //
    //     if (not barriers.empty())
    //     {
    //         commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
    //     }
    // }

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
        commandList->CopyResource(m_readbackBuffer.Get(), m_gpuBuffer.Get());

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

            commandList->CopyResource(impl->m_readbackBuffer.Get(), impl->m_gpuBuffer.Get());
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

        uint8_t* src = nullptr;
        if (SUCCEEDED(m_readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&src))))
        {
            memcpy(dest, src, m_dataSize);
            m_readbackBuffer->Unmap(0, nullptr);
        }
        else
        {
            LogError.writeln("StructuredBufferTransfer::Readback(): Failed to map readback buffer.");
        }
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
