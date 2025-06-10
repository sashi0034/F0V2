#include "pch.h"
#include "IndexBuffer.h"

#include "Logger.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

struct IndexBuffer::Impl
{
    bool m_valid{};
    ComPtr<ID3D12Resource> m_indexBuffer{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    Impl(int count)
    {
        const auto device = EngineRenderContext::GetDevice();

        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(index_type) * count);

        // リソース作成
        if (const auto hr = device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_indexBuffer));
            FAILED(hr))
        {
            LogError.writeln(L"IndexBuffer: Failed to create buffer");
            return;
        }

        m_indexBuffer->SetName(L"IndexBuffer");

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = resourceDesc.Width;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;

        m_valid = true;
    }

    void Upload(const Array<index_type>& indices)
    {
        index_type* indexMap{};

        if (const auto hr = m_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));
            FAILED(hr))
        {
            LogError.writeln(L"IndexBuffer: Failed to map index buffer");
            return;
        }

        std::ranges::copy(indices, indexMap);

        m_indexBuffer->Unmap(0, nullptr); // TODO: Unmap タイミング調整
    }

    void CommandSet() const
    {
        const auto commandList = EngineRenderContext::GetCommandList();
        commandList->IASetIndexBuffer(&m_indexBufferView);
    }
};

namespace TY
{
    IndexBuffer::IndexBuffer()
    {
    }

    IndexBuffer::IndexBuffer(int count) : p_impl(std::make_shared<Impl>(count))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
            return;
        }
    }

    IndexBuffer::IndexBuffer(const Array<index_type>& indices) : p_impl(std::make_shared<Impl>(indices.size()))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
            return;
        }

        p_impl->Upload(indices);
    }

    void IndexBuffer::upload(const Array<index_type>& indices)
    {
        if (not p_impl) return;
        p_impl->Upload(indices);
    }

    void IndexBuffer::commandSet() const
    {
        if (not p_impl) return;
        p_impl->CommandSet();
    }

    int IndexBuffer::count() const
    {
        if (not p_impl) return {};
        return p_impl->m_indexBufferView.SizeInBytes / sizeof(index_type);
    }
}
