#pragma once
#include "DescriptorTable.h"
#include "PipelineType.h"
#include "TY/ShaderResourceTexture.h"
#include "TY/ConstantBufferUploader.h"
#include "TY/UnorderedAccessTransfer.h"

namespace TY::detail
{
    using ShaderResourceType = Variant<ShaderResourceTexture, UnorderedAccessTransfer>;

    struct CbSrUaSet
    {
        Array<ConstantBufferUploader_impl> cb; /* [cbvCount] where ConstantBuffer::count() = materialCount */
        Array<Array<ShaderResourceType>> sr; /* [srvCount][materialCount] */
        Array<Array<UnorderedAccessTransfer>> ua; /* [uavCount][materialCount] */
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

        void commandSet() const;

        void commandSetTable(PipelineType pipeline, int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
