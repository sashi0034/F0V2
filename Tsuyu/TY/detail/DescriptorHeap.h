#pragma once
#include "DescriptorTable.h"
#include "PipelineType.h"
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBufferUploader.h"

namespace TY::detail
{
    struct CbvSrvUavSet
    {
        Array<ConstantBufferUploader_impl> cbv; /* [cbvCount], ConstantBuffer::count() = materialCount */
        Array<Array<ShaderResourceType>> srv; /* [srvCount][materialCount] */
        Array<Array<UnorderedAccessType>> uav; /* [uavCount][materialCount] */
    };

    struct DescriptorHeapParams
    {
        DescriptorTable table;
        Array<size_t> materialCounts;
        Array<CbvSrvUavSet> descriptors;
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap() = default;

        DescriptorHeap(const DescriptorHeapParams& params);

        void resetSrv(const ShaderResourceType& srv, int tableId, int srvId, int materialId = 0);

        void resetUav(const UnorderedAccessType& uav, int tableId, int uavId, int materialId = 0);

        void commandSet() const;

        void commandSetTable(PipelineType pipeline, int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
