#include "pch.h"
#include "Buffer3D.h"

#include "AssertObject.h"
#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"

using namespace TY;
using namespace TY::detail;

namespace
{
}

struct Buffer3D::Impl
{
    ComPtr<ID3D12Resource> m_vertBuffer{};
    ComPtr<ID3D12Resource> m_indexBuffer{};

    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

    // TODO: Close?

    Impl(const Buffer3DParams& params)
    {
        const auto device = EngineRenderContext::GetDevice();

        D3D12_HEAP_PROPERTIES heapProperties = {};
        heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN; // GPU メモリ領域における CPU のアクセス方法
        heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = params.vertexes.size_in_bytes();
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        // アップロード
        AssertWin32{"failed to create buffer"sv}
            | device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertBuffer)
            );

        // バッファの仮想アドレスを取得
        Vertex* vertMap{};
        AssertWin32{"failed to map vertex buffer"sv}
            | m_vertBuffer->Map(0, nullptr, reinterpret_cast<void**>(&vertMap));

        // マップしたメモリ位置へデータを転送
        std::ranges::copy(params.vertexes, vertMap);

        // アンマップ
        m_vertBuffer->Unmap(0, nullptr);

        m_vertexBufferView.BufferLocation = m_vertBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.SizeInBytes = params.vertexes.size_in_bytes();
        m_vertexBufferView.StrideInBytes = sizeof(params.vertexes[0]);

        // -----------------------------------------------

        resourceDesc.Width = sizeof(params.indices[0]) * params.indices.size();
        AssertWin32{"failed to create buffer"sv}
            | device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_indexBuffer)
            );

        // バッファの仮想アドレスを取得
        uint16_t* indexMap{};
        AssertWin32{"failed to map index buffer"sv}
            | m_indexBuffer->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));

        // マップしたメモリ位置へデータを転送
        std::ranges::copy(params.indices, indexMap);

        // アンマップ
        m_indexBuffer->Unmap(0, nullptr);

        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.SizeInBytes = params.indices.size_in_bytes();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    }

    void Draw() const
    {
        const auto commandList = EngineRenderContext::GetCommandList();

        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->IASetIndexBuffer(&m_indexBufferView);

        commandList->DrawIndexedInstanced(m_indexBufferView.SizeInBytes / sizeof(uint16_t), 1, 0, 0, 0);
    }
};

namespace TY
{
    Buffer3D::Buffer3D(const Buffer3DParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    void Buffer3D::Draw() const
    {
        p_impl->Draw();
    }
}
