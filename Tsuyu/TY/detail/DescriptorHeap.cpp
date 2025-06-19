#include "pch.h"
#include "DescriptorHeap.h"

#include "EngineCore.h"
#include "EnginePresetAsset.h"
#include "EngineRenderContext.h"
#include "TY/Logger.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    int countDescriptors(const DescriptorHeapParams& params)
    {
        int count{};

        const bool ok =
            params.table.size() == params.materialCounts.size() &&
            params.table.size() == params.descriptors.size();
        if (not ok)
        {
            LogError(std::format(
                "DescriptorHeap: Table size mismatch: table.size()={}, materialCounts.size()={}, descriptors.size()={}",
                params.table.size(),
                params.materialCounts.size(),
                params.descriptors.size()));
            return 0;
        }

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

    bool checkTableValid(const DescriptorHeapParams& params, int tableId)
    {
        if (params.descriptors[tableId].cb.size() != params.table[tableId].cbvCount)
        {
            LogError(std::format(
                "DescriptorHeap: Constant buffer count mismatch for table {}: {} != {}",
                tableId,
                params.descriptors[tableId].cb.size(),
                params.table[tableId].cbvCount));
            return false;
        }

        if (params.descriptors[tableId].sr.size() != params.table[tableId].srvCount)
        {
            LogError(std::format(
                "DescriptorHeap: Shader resource count mismatch for table {}: {} != {}",
                tableId,
                params.descriptors[tableId].sr.size(),
                params.table[tableId].srvCount));
            return false;
        }

        if (params.descriptors[tableId].ua.size() != params.table[tableId].uavCount)
        {
            LogError(std::format(
                "DescriptorHeap: Unordered access count mismatch for table {}: {} != {}",
                tableId,
                params.descriptors[tableId].ua.size(),
                params.table[tableId].uavCount));
            return false;
        }

        return true;
    }

    bool createConstantBufferView(
        D3D12_CPU_DESCRIPTOR_HANDLE heapHandle,
        int tableId,
        int cbvId,
        int materialId,
        const DescriptorHeapParams& params)
    {
        const auto& cb = params.descriptors[tableId].cb[cbvId];
        if (not cb.isEmpty() && cb.materialCount() != params.materialCounts[tableId])
        {
            LogError(std::format(
                "DescriptorHeap: Constant buffer count mismatch: {} != {}",
                cb.materialCount(),
                params.materialCounts[tableId]));
            return false;
        }

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = cb.bufferLocation() + materialId * cb.alignedSize();
        cbvDesc.SizeInBytes = static_cast<UINT>(cb.alignedSize());
        EngineRenderContext::GetDevice()->CreateConstantBufferView(&cbvDesc, heapHandle);

        return true;
    }

    bool createShaderResourceViewInternal(D3D12_CPU_DESCRIPTOR_HANDLE heapHandle, const ShaderResourceType& sr)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        ID3D12Resource* p_resource{};
        if (sr.isHolds<ShaderResourceTexture>())
        {
            const auto& t = sr.get<ShaderResourceTexture>();
            const auto materialSR =
                t.isEmpty() ? EnginePresetAsset::GetWhiteTexture() : t;

            srvDesc.Format = t.getFormat();
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MipLevels = 1;

            p_resource = materialSR.getResource();
        }
        else if (sr.isHolds<StructuredBufferUploader>())
        {
            const auto& t = sr.get<StructuredBufferUploader>();

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Buffer.FirstElement = 0;
            srvDesc.Buffer.NumElements = t.elementCount();
            srvDesc.Buffer.StructureByteStride = t.elementStride();
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

            p_resource = t.getBuffer();
        }

        assert(p_resource);

        EngineRenderContext::GetDevice()->CreateShaderResourceView(p_resource, &srvDesc, heapHandle);
        return true;
    }

    bool createShaderResourceView(
        D3D12_CPU_DESCRIPTOR_HANDLE heapHandle,
        int tableId,
        int srvId,
        int materialId,
        const DescriptorHeapParams& params)
    {
        const auto& srArray = params.descriptors[tableId].sr[srvId];
        if (srArray.size() != params.materialCounts[tableId])
        {
            LogError(std::format(
                "DescriptorHeap: Shader resource elements count mismatch: {} != {}",
                srArray.size(),
                params.materialCounts[tableId]));
            return false;
        }

        const auto sr = srArray[materialId];
        return createShaderResourceViewInternal(heapHandle, sr);
    }

    bool createUnorderedAccessViewInternal(D3D12_CPU_DESCRIPTOR_HANDLE heapHandle, const StructuredBufferUploader& ua)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uavDesc.Buffer.FirstElement = 0;
        uavDesc.Buffer.NumElements = ua.elementCount();
        uavDesc.Buffer.StructureByteStride = ua.elementStride();

        EngineRenderContext::GetDevice()->CreateUnorderedAccessView(
            ua.getBuffer(),
            nullptr,
            &uavDesc,
            heapHandle);
        return true;
    }

    bool createUnorderedAccessView(
        D3D12_CPU_DESCRIPTOR_HANDLE heapHandle,
        int tableId,
        int uavId,
        int materialId,
        const DescriptorHeapParams& params)
    {
        const auto& uaArray = params.descriptors[tableId].ua[uavId];
        if (uaArray.size() != params.materialCounts[tableId])
        {
            LogError(std::format(
                "DescriptorHeap: Unordered access elements count mismatch: {} != {}",
                uaArray.size(),
                params.materialCounts[tableId]));
            return false;
        }

        const auto ua = uaArray[materialId];
        return createUnorderedAccessViewInternal(heapHandle, ua);
    }

    UINT getHandleIncrementalSize()
    {
        return EngineRenderContext::GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

struct DescriptorHeap::Impl
{
    bool m_valid{};

    int m_descriptorsCount{};

    ComPtr<ID3D12DescriptorHeap> m_descriptorHeap{};

    Array<CbSrUaSet> m_descriptors{}; // for resource lifetime

    Array<Array<size_t>> m_handleOffsets{}; // tableId, materialId

    Impl(const DescriptorHeapParams& params)
    {
        m_descriptors = params.descriptors;
        m_descriptorsCount = countDescriptors(params);
        if (m_descriptorsCount == 0)
        {
            LogError.writeln("DescriptorHeap: No descriptors to create.");
            return;
        }

        // ディスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descriptorHeapDesc.NodeMask = 0;
        descriptorHeapDesc.NumDescriptors = m_descriptorsCount;
        descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

        if (const HRESULT hr = EngineRenderContext::GetDevice()->CreateDescriptorHeap(
                &descriptorHeapDesc,
                IID_PPV_ARGS(&m_descriptorHeap));
            FAILED(hr))
        {
            LogError.writeln(std::format("DescriptorHeap: Failed to create descriptor heap: {}", hr));
            return;
        }

        m_descriptorHeap->SetName(L"DescriptorHeap");

        const auto heapHandleStart = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        auto heapHandle = heapHandleStart;
        const auto incrementSize = getHandleIncrementalSize();

        // ビュー登録
        m_handleOffsets.resize(params.table.size());
        for (int tableId = 0; tableId < params.table.size(); ++tableId)
        {
            if (not checkTableValid(params, tableId))
            {
                return;
            }

            auto& handles = m_handleOffsets[tableId];
            for (int materialId = 0; materialId < params.materialCounts[tableId]; ++materialId)
            {
                handles.push_back(heapHandle.ptr - heapHandleStart.ptr);

                // CBV
                for (int cbvId = 0; cbvId < params.table[tableId].cbvCount; ++cbvId)
                {
                    if (not createConstantBufferView(heapHandle, tableId, cbvId, materialId, params)) return;

                    heapHandle.ptr += incrementSize;
                }

                // SRV
                for (int srvId = 0; srvId < params.table[tableId].srvCount; ++srvId)
                {
                    if (not createShaderResourceView(heapHandle, tableId, srvId, materialId, params)) return;

                    heapHandle.ptr += incrementSize;
                }

                // UAV
                for (int uavId = 0; uavId < params.table[tableId].uavCount; ++uavId)
                {
                    if (not createUnorderedAccessView(heapHandle, tableId, uavId, materialId, params)) return;

                    heapHandle.ptr += incrementSize;
                }
            }
        }

        m_valid = true;
    }

    void ResetSRV(const ShaderResourceType& sr, int tableId, int srvId, int materialId)
    {
        auto heapHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        heapHandle.ptr += m_handleOffsets[tableId][materialId];

        const auto incrementSize =
            EngineRenderContext::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // CBV
        heapHandle.ptr += incrementSize * m_descriptors[tableId].cb.size();

        // SRV
        heapHandle.ptr += incrementSize * srvId;

        createShaderResourceViewInternal(heapHandle, sr);
    }

    void ResetUAV(const UnorderedAccessType& ua, int tableId, int uavId, int materialId)
    {
        auto heapHandle = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        heapHandle.ptr += m_handleOffsets[tableId][materialId];

        const auto incrementSize =
            EngineRenderContext::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        // CBV
        heapHandle.ptr += incrementSize * m_descriptors[tableId].cb.size();

        // SRV
        heapHandle.ptr += incrementSize * m_descriptors[tableId].sr.size();

        // UAV
        heapHandle.ptr += incrementSize * uavId;

        createUnorderedAccessViewInternal(heapHandle, ua);
    }

    void CommandSet() const
    {
        EngineRenderContext::ActiveCommandList()->SetDescriptorHeaps(1, m_descriptorHeap.GetAddressOf());
    }

    void CommandSetTable(PipelineType pipeline, int tableId, int materialId) const
    {
        auto heapHandle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        heapHandle.ptr += m_handleOffsets[tableId][materialId];

        if (pipeline == PipelineType::Graphics)
        {
            EngineRenderContext::ActiveCommandList()->SetGraphicsRootDescriptorTable(tableId, heapHandle);
        }
        else if (pipeline == PipelineType::Compute)
        {
            EngineRenderContext::ActiveCommandList()->SetComputeRootDescriptorTable(tableId, heapHandle);
        }
        else
        {
            assert(false);
        }
    }
};

namespace TY::detail
{
    DescriptorHeap::DescriptorHeap(const DescriptorHeapParams& params) : p_impl(std::make_shared<Impl>(params))
    {
        if (not p_impl->m_valid)
        {
            p_impl.reset();
        }
    }

    void DescriptorHeap::resetSRV(const ShaderResourceType& sr, int tableId, int srvId, int materialId)
    {
        if (p_impl) p_impl->ResetSRV(sr, tableId, srvId, materialId);
    }

    void DescriptorHeap::resetUAV(const UnorderedAccessType& ua, int tableId, int uavId, int materialId)
    {
        if (p_impl) p_impl->ResetUAV(ua, tableId, uavId, materialId);
    }

    void DescriptorHeap::commandSet() const
    {
        if (p_impl) p_impl->CommandSet();
    }

    void DescriptorHeap::commandSetTable(PipelineType pipeline, int tableId, int materialId) const
    {
        if (p_impl) p_impl->CommandSetTable(pipeline, tableId, materialId);
    }
}
