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
    size_t m_dataSize{};

    Impl(const UnorderedAccessUploaderParams& params) : m_params(params)
    {
        const auto device = EngineRenderContext::GetDevice();

        m_dataSize = params.elementCount * params.elementStride;

        const CD3DX12_RESOURCE_DESC gpuBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            sizeof(uint32_t) * m_dataSize,
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

        // upload buffer にデータを書き込む
        const CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            sizeof(uint32_t) * m_dataSize,
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
