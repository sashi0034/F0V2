#include "pch.h"
#include "DescriptorHeap.h"

#include "EngineCore.h"
#include "TY/AssertObject.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    int countDescriptors(const DescriptorHeapParams& params)
    {
        int count{};

        AssertTrue{"descriptor table size mismatch"sv}
            | params.table.size() == params.materialCounts.size()
            | params.table.size() == params.descriptors.size();

        for (int i = 0; i < params.table.size(); ++i)
        {
            const int cb = params.table[i].cbvCount * params.materialCounts[i];
            count += cb;

            const int sr = params.table[i].srvCount * params.materialCounts[i];
            count += sr;

            const int ua = params.table[i].uavCount * params.materialCounts[i];
            count += ua;
        }

        return count;
    }
}

struct DescriptorHeap::Impl
{
    int m_descriptorsCount{};

    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap{};

    Array<CbSrUaSet> m_descriptors{}; // for resource lifetime

    Array<Array<size_t>> m_handleOffsets{}; // tableId, materialId

    Impl(const DescriptorHeapParams& params)
    {
        m_descriptors = params.descriptors;
        m_descriptorsCount = countDescriptors(params);

        // ディスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descriptorHeapDesc.NodeMask = 0;
        descriptorHeapDesc.NumDescriptors = m_descriptorsCount;
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        AssertWin32{"failed to create descriptor heap"sv}
            | EngineCore.GetDevice()->CreateDescriptorHeap(
                &descriptorHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
        m_descriptorHeap->SetName(L"DescriptorHeap");

        const auto heapHandleStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        auto heapHandle = heapHandleStart;
        const auto incrementSize =
            EngineCore.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // ビュー登録
        m_handleOffsets.resize(params.table.size());
        for (int tableId = 0; tableId < params.table.size(); ++tableId)
        {
            auto& handles = m_handleOffsets[tableId];
            for (int materialId = 0; materialId < params.materialCounts[tableId]; ++materialId)
            {
                handles.push_back(heapHandle.ptr - heapHandleStart.ptr);

                // CBV
                AssertWin32{"constant buffer size mismatch"sv} // FIXME: 丁寧なメッセージ
                    | params.descriptors[tableId].cb.size() == params.table[tableId].cbvCount;
                for (int cavId = 0; cavId < params.table[tableId].cbvCount; ++cavId)
                {
                    const auto& cb = params.descriptors[tableId].cb[cavId];
                    AssertWin32{"constant buffer elements count mismatch"sv}
                        | cb.count() == params.materialCounts[tableId];

                    AssertTrue{"constant buffer is empty"sv}
                        | cb.alignedSize() != 0;

                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                    cbvDesc.BufferLocation = cb.bufferLocation() + materialId * cb.alignedSize();
                    cbvDesc.SizeInBytes = static_cast<UINT>(cb.alignedSize());
                    EngineCore.GetDevice()->CreateConstantBufferView(
                        &cbvDesc, heapHandle);

                    heapHandle.ptr += incrementSize;
                }

                // SRV
                AssertWin32{"failed to create descriptor heap"sv}
                    | params.descriptors[tableId].sr.size() == params.table[tableId].srvCount;
                for (int srvId = 0; srvId < params.table[tableId].srvCount; ++srvId)
                {
                    const auto& sr = params.descriptors[tableId].sr[srvId];
                    AssertWin32{"constant buffer elements count mismatch"sv}
                        | sr.size() == params.materialCounts[tableId];

                    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                    srvDesc.Format = sr[materialId].getFormat();
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    srvDesc.Texture2D.MipLevels = 1;

                    EngineCore.GetDevice()->CreateShaderResourceView(
                        sr[materialId].getResource(), &srvDesc, heapHandle);

                    heapHandle.ptr += incrementSize;
                }

                // UAV
                // TODO
            }
        }
    }

    void CommandSet() const
    {
        EngineCore.GetCommandList()->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    }

    void CommandSetTable(int tableId, int materialId) const
    {
        auto heapHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        heapHandle.ptr += m_handleOffsets[tableId][materialId];

        EngineCore.GetCommandList()->SetGraphicsRootDescriptorTable(tableId, heapHandle);
    }
};

namespace TY::detail
{
    DescriptorHeap::DescriptorHeap(const DescriptorHeapParams& params) : p_impl(std::make_shared<Impl>(params))
    {
    }

    void DescriptorHeap::CommandSet() const
    {
        if (p_impl) p_impl->CommandSet();
    }

    void DescriptorHeap::CommandSetTable(int tableId, int materialId) const
    {
        if (p_impl) p_impl->CommandSetTable(tableId, materialId);
    }
}
