#pragma once
#include "DescriptorEntry.h"
#include "DynamicDescriptorEntry.h"
#include "TY/GraphicsOptions.h"

namespace TY::detail
{
    struct RootSignatureParams
    {
        Array<GraphicsSamplerOptions> samplers;
        Array<DescriptorEntry> descriptorTable;
        Array<DynamicDescriptorEntry> dynamicDescriptorTable{};
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

        int dynamicBindingRootParameterOffset() const
        {
            return m_dynamicBindingRootParameterOffset;
        }

        const Array<DynamicDescriptorEntry>& resolvedDynamicDescriptorTable() const
        {
            return m_resolvedDynamicDescriptorTable;
        }

    private:
        ComPtr<ID3D12RootSignature> m_rootSignature{};
        int m_dynamicBindingRootParameterOffset{};
        Array<DynamicDescriptorEntry> m_resolvedDynamicDescriptorTable{};
    };
}
