#include "pch.h"
#include "UnorderedAccessUploader.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct UnorderedAccessUploader::Impl
{
    UnorderedAccessUploaderParams m_params;
    ComPtr<ID3D12Resource> m_gpuBuffer;
    ComPtr<ID3D12Resource> m_uploadBuffer;
    ComPtr<ID3D12Resource> m_readbackBuffer;
    size_t m_dataSize{};

    Impl(const UnorderedAccessUploaderParams& params) : m_params(params)
    {
        const auto device = EngineRenderContext::GetDevice();

        m_dataSize = params.elementCount * params.elementStride;

        const CD3DX12_RESOURCE_DESC gpuBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            m_dataSize,
            D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        );

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

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
            LogError.writeln(std::format("Failed to create GPU buffer: {}", hr));
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
            LogError.writeln(std::format("Failed to create upload buffer: {}", hr));
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
            LogError.writeln(std::format("Failed to create readback buffer: {}", hr));
            return;
        }
    }

    void Upload(const uint8_t* src)
    {
        // マップして書き込み
        uint8_t* dest;

        if (const HRESULT hr = m_uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&dest));
            FAILED(hr))
        {
            LogError.writeln(std::format("Failed to map upload buffer: {}", hr));
            return;
        }

        memcpy(dest, src, m_dataSize);
        m_uploadBuffer->Unmap(0, nullptr);

        // GPU へアップロード
        const auto commandList = EngineRenderContext::GetCommandList();
        commandList->CopyResource(m_gpuBuffer.Get(), m_uploadBuffer.Get());

        // CopyResource で COPY_DEST 状態になっている m_gpuBuffer を、UNORDERED_ACCESS に移す
        const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_gpuBuffer.Get(),
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrier);
    }

    void Readback(uint8_t* dest)
    {
        const auto commandList = EngineRenderContext::GetCommandList();

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

        EngineRenderContext::CloseAndFlush();

        uint8_t* src = nullptr;
        if (SUCCEEDED(m_readbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&src))))
        {
            memcpy(dest, src, m_dataSize);
            m_readbackBuffer->Unmap(0, nullptr);

            for (size_t i = 0; i < min(m_dataSize, size_t(64)); ++i)
            {
                printf("%02X ", src[i]);
            }
            printf("\n");
        }
        else
        {
            LogError.writeln("Failed to map readback buffer.");
        }
    }
};

namespace TY
{
    UnorderedAccessUploader::UnorderedAccessUploader(const UnorderedAccessUploaderParams& params)
        : p_impl(std::make_shared<Impl>(params))
    {
    }

    void UnorderedAccessUploader::upload(const void* src)
    {
        if (p_impl) p_impl->Upload(static_cast<const uint8_t*>(src));
    }

    void UnorderedAccessUploader::readback(void* dst)
    {
        if (p_impl) p_impl->Readback(static_cast<uint8_t*>(dst));
    }

    int UnorderedAccessUploader::elementCount() const
    {
        return p_impl ? p_impl->m_params.elementCount : 0;
    }

    int UnorderedAccessUploader::elementStride() const
    {
        return p_impl ? p_impl->m_params.elementStride : 0;
    }

    ID3D12Resource* UnorderedAccessUploader::getBuffer() const
    {
        return p_impl ? p_impl->m_gpuBuffer.Get() : nullptr;
    }
}
