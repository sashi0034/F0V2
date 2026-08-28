#pragma once
#include "DescriptorTable.h"
#include "DynamicDescriptorTable.h"
#include "TY/GraphicsOptions.h"

namespace TY::detail
{
    struct RootSignatureParams
    {
        Array<GraphicsSamplerOptions> samplers;
        Array<DescriptorTableElement> descriptorTable;
        Array<DynamicDescriptorTableElement> dynamicDescriptors{};
    };

    class RootSignature
    {
    public:
        RootSignature() = default;

        RootSignature(const RootSignatureParams& params);

        const ComPtr<ID3D12RootSignature>& get() const
        {
            return m_rootSignature;
        }

        ID3D12RootSignature* getPointer() const
        {
            return m_rootSignature.Get();
        }

    private:
        ComPtr<ID3D12RootSignature> m_rootSignature{};
    };
}
