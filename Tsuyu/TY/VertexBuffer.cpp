#include "pch.h"
#include "VertexBuffer.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct VertexBufferCore::Impl
{
    bool m_valid{};

    D3D12_VERTEX_BUFFER_VIEW m_vertBufferView{};

    ComPtr<ID3D12Resource> m_finalBuffer{};

    int m_count{};

    struct frame_resources
    {
        ComPtr<ID3D12Resource> uploadBuffer;
        uint8_t* dest{};
    };

    std::array<frame_resources, EngineRenderContext::FrameBufferCount> m_frameResources{};

    size_t m_uploadTimestamp{};

    Impl(int sizeInBytes, int strideInBytes)
    {
        const auto device = EngineRenderContext::GetDevice();

        const D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

        // リソース作成
        if (const auto hr = device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&m_finalBuffer));
            FAILED(hr))
        {
            LogError.writeln("VertexBuffer: Failed to create m_finalBuffer");
            return;
        }

        m_finalBuffer->SetName(L"VertexBuffer::m_finalBuffer");

        m_vertBufferView.BufferLocation = m_finalBuffer->GetGPUVirtualAddress();
        m_vertBufferView.SizeInBytes = sizeInBytes;
        m_vertBufferView.StrideInBytes = strideInBytes;

        m_count = sizeInBytes / strideInBytes;

        m_valid = true;
    }

    ~Impl()
    {
        for (auto& frameResource : m_frameResources)
        {
            if (frameResource.uploadBuffer && frameResource.dest)
            {
                frameResource.uploadBuffer->Unmap(0, nullptr);
            }

            EngineRenderContext::SafeDisposeRenderResource(frameResource.uploadBuffer);
        }

        EngineRenderContext::SafeDisposeRenderResource(m_finalBuffer);
    }

    void Upload(const void* data, size_t size)
    {
        const size_t previousUploadTimestamp = m_uploadTimestamp;
        m_uploadTimestamp = EngineRenderContext::GetFlushTimestamp();

        const size_t frameIndex = m_uploadTimestamp % EngineRenderContext::FrameBufferCount;

        auto& frameResource = m_frameResources[frameIndex];

        if (not frameResource.uploadBuffer)
        {
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertBufferView.SizeInBytes);

            if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&frameResource.uploadBuffer));
                FAILED(hr))
            {
                LogError.writeln("VertexBuffer: Failed to create uploadBuffer.");
                return;
            }

            frameResource.uploadBuffer->SetName(L"VertexBuffer::uploadBuffer");
        }

        if (not frameResource.dest)
        {
            if (const HRESULT hr = frameResource.uploadBuffer->Map(
                    0, nullptr, reinterpret_cast<void**>(&frameResource.dest));
                FAILED(hr))
            {
                LogError.writeln("VertexBuffer: Failed to map uploadBuffer");
                return;
            }
        }

        uint8_t* dest = frameResource.dest;
        memcpy(dest, data, size);

        const auto copyCommandList = EngineRenderContext::GetCommandList(CommandListType::Copy);
        copyCommandList->CopyBufferRegion(
            m_finalBuffer.Get(),
            0,
            frameResource.uploadBuffer.Get(),
            0,
            size);

        if (previousUploadTimestamp == 0)
        {
            // 初回実行時は即アンマップする
            frameResource.uploadBuffer->Unmap(0, nullptr);
            frameResource.dest = nullptr;
        }
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::GetCommandList(CommandListType::Draw);
        commandList->IASetVertexBuffers(0, 1, &m_vertBufferView);
    }
};

namespace TY
{
    VertexBufferCore::VertexBufferCore(int sizeInBytes, int strideInBytes) :
        p_impl(std::make_shared<Impl>(sizeInBytes, strideInBytes))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    bool VertexBufferCore::isEmpty() const
    {
        return p_impl == nullptr;
    }

    int VertexBufferCore::count() const
    {
        return p_impl ? p_impl->m_count : 0;
    }

    void VertexBufferCore::upload(const void* data)
    {
        if (not p_impl) return;
        p_impl->Upload(data, p_impl->m_vertBufferView.SizeInBytes);
    }

    void VertexBufferCore::upload(const void* data, int count)
    {
        if (not p_impl) return;
        p_impl->Upload(data, p_impl->m_vertBufferView.StrideInBytes * count);
    }

    void VertexBufferCore::commandSet() const
    {
        if (not p_impl) return;
        p_impl->CommandSet();
    }
}
