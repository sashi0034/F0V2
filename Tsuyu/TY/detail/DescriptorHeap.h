#pragma once
#include "CommandListManager.h"
#include "DescriptorTable.h"
#include "PipelineType.h"
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBufferArray.h"

namespace TY::detail
{
    struct CbvSrvUavSet
    {
        // TODO: それぞれ配列をクラス派生したい

        /// @remark [cbvCount], ConstantBuffer::count() = materialCount
        Array<ConstantBufferArrayImpl> cbv;

        /// @remark [materialCount][srvCount]
        Array<Array<ShaderResourceType>> srv;

        /// @remark [materialCount][uavCount]
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

        /// @remark 空の場合のみ登録可能
        void registerSrv(const ShaderResourceType& srv, int tableId, int srvId, int materialId = 0);

        void registerUav(const UnorderedAccessType& uav, int tableId, int uavId, int materialId = 0);

        void commandSet() const;

        void commandSetGraphicsTable(int tableId, int materialId = 0) const;

        void commandSetComputeTable(int tableId, int materialId = 0) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
