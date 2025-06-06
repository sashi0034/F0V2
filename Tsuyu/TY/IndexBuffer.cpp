#include "pch.h"
#include "IndexBuffer.h"

#include "AssertObject.h"
#include "detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

struct IndexBuffer::Impl
{
    ComPtr<ID3D12Resource> m_indexBuffer{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    Impl(int count)
    {
        const auto device = EngineCore.GetDevice();

        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(index_type) * count);

        // リソース作成
        AssertWin32{"failed to create buffer"sv}
            | device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_indexBuffer)
            );

        m_indexBuffer->SetName(L"IndexBuffer");

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = resourceDesc.Width;
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    }

    void Upload(const Array<index_type>& indices)
    {
        index_type* indexMap{};
        AssertWin32{"failed to map index buffer"sv}
            | m_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));

        std::ranges::copy(indices, indexMap);

        m_indexBuffer->Unmap(0, nullptr); // TODO: Unmap タイミング調整
    }

    void CommandSet() const
    {
        const auto commandList = EngineCore.GetCommandList();
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
    }

    IndexBuffer::IndexBuffer(const Array<index_type>& indices) : p_impl(std::make_shared<Impl>(indices.size()))
    {
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
