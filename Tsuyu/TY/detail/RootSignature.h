#pragma once
#include "DescriptorTable.h"

namespace TY::detail
{
    struct RootSignatureParams
    {
        Array<DescriptorTableElement> descriptorTable;
    };

    class RootSignature
    {
    public:
        RootSignature() = default;

        RootSignature(const RootSignatureParams& params);

        ID3D12RootSignature* getPointer() const
        {
            return m_rootSignature.Get();
        }

    private:
        ComPtr<ID3D12RootSignature> m_rootSignature{};
    };
}
