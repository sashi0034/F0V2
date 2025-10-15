#pragma once
#include "CommandList.h"
#include "DescriptorTable.h"
#include "PipelineType.h"
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBuffer.h"

namespace TY::detail
{
    struct CbvSrvUavSet
    {
        /// @remark [cbvCount], ConstantBuffer::count() = materialCount
        Array<ConstantBufferCore> cbv;

        /// @remark [srvCount][materialCount]
        Array<Array<ShaderResourceType>> srv;

        /// @remark [uavCount][materialCount]
        Array<Array<UnorderedAccessType>> uav;
    };

    struct DescriptorHeapParams
    {
        DescriptorTable table;
        Array<int> materialCounts;
        Array<CbvSrvUavSet> descriptors;
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap() = default;

        DescriptorHeap(const DescriptorHeapParams& params);

        void resetSrv(const ShaderResourceType& srv, int tableId, int srvId, int materialId = 0);

        void resetUav(const UnorderedAccessType& uav, int tableId, int uavId, int materialId = 0);

        void commandSet(CommandListType commandList) const;

        void commandSetGraphicsTable(CommandListType commandList, int tableId, int materialId = 0) const;

        void commandSetComputeTable(CommandListType commandList, int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
