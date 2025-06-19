#pragma once
#include "DescriptorTable.h"
#include "PipelineType.h"
#include "TY/ShaderResourceTexture.h"
#include "TY/ConstantBufferUploader.h"
#include "TY/StructuredBufferUploader.h"

namespace TY::detail
{
    using ShaderResourceType = Variant<ShaderResourceTexture, StructuredBufferUploader>;

    using UnorderedAccessType = StructuredBufferTransfer;

    struct CbSrUaSet
    {
        Array<ConstantBufferUploader_impl> cb; /* [cbvCount], ConstantBuffer::count()=materialCount */
        Array<Array<ShaderResourceType>> sr; /* [srvCount][materialCount] */
        Array<Array<UnorderedAccessType>> ua; /* [uavCount][materialCount] */
    };

    struct DescriptorHeapParams
    {
        DescriptorTable table;
        Array<size_t> materialCounts;
        Array<CbSrUaSet> descriptors;
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap() = default;

        DescriptorHeap(const DescriptorHeapParams& params);

        void resetSRV(const ShaderResourceType& sr, int tableId, int srvId, int materialId = 0);

        void resetUAV(const UnorderedAccessType& ua, int tableId, int uavId, int materialId = 0);

        void commandSet() const;

        void commandSetTable(PipelineType pipeline, int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
