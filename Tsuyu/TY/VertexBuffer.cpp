#include "pch.h"
#include "VertexBuffer.h"

#include "AssertObject.h"
#include "detail/EngineCore.h"

using namespace TY;
using namespace TY::detail;

struct VertexBuffer_impl::Impl
{
    ComPtr<ID3D12Resource> m_vertBuffer{};
    D3D12_VERTEX_BUFFER_VIEW m_vertBufferView{};

    Impl(int sizeInBytes, int strideInBytes)
    {
        const auto device = EngineCore::GetDevice();

        const D3D12_HEAP_PROPERTIES heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        const D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);

        // リソース作成
        AssertWin32{"failed to create buffer"sv}
            | device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertBuffer)
            );

        m_vertBuffer->SetName(L"VertexBuffer");

        m_vertBufferView.BufferLocation = m_vertBuffer->GetGPUVirtualAddress();
        m_vertBufferView.SizeInBytes = sizeInBytes;
        m_vertBufferView.StrideInBytes = strideInBytes;
    }

    void Upload(const void* data)
    {
        void* p;
        AssertWin32{"failed to map"sv}
            | m_vertBuffer->Map(0, nullptr, &p);

        memcpy(p, data, m_vertBufferView.SizeInBytes);

        m_vertBuffer->Unmap(0, nullptr); // TODO: Unmap のタイミング調整
    }

    void CommandSet() const
    {
        const auto commandList = EngineCore::GetCommandList();
        commandList->IASetVertexBuffers(0, 1, &m_vertBufferView);
    }
};

namespace TY
{
    VertexBuffer_impl::VertexBuffer_impl(int sizeInBytes, int strideInBytes) :
        p_impl(std::make_shared<Impl>(sizeInBytes, strideInBytes))
    {
    }

    void VertexBuffer_impl::upload(const void* data)
    {
        if (not p_impl) return;
        p_impl->Upload(data);
    }

    void VertexBuffer_impl::commandSet() const
    {
        if (not p_impl) return;
        p_impl->CommandSet();
    }
}
