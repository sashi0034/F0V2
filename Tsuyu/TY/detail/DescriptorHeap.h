#pragma once
#include "DescriptorEntry.h"
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBuffer.h"
#include "TY/HybridArray.h"
#include "TY/MaterialList.h"

namespace TY::detail
{
    struct CbvSrvUavSet
    {
        /// @remark [materialCount][cbvCount]
        MaterialList<DescriptorList<ConstantBufferImpl>> cbv;

        /// @remark [materialCount][srvCount]
        MaterialList<DescriptorList<ShaderResourceType>> srv;

        /// @remark [materialCount][uavCount]
        MaterialList<DescriptorList<UnorderedAccessType>> uav;
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
