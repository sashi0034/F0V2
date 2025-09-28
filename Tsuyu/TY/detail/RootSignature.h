#pragma once
#include "DescriptorTable.h"
#include "ShaderRegisterStart.h"
#include "TY/GraphicsOptions.h"
#include "TY/Uncopyable.h"

namespace TY::detail
{
    struct RootSignatureParams
    {
        Array<GraphicsSamplerOptions> samplers;
        Array<DescriptorTableElement> descriptorTable;
        Array<ShaderRegisterStart> explicitRegisterStarts;
    };

    class RootSignature : Uncopyable
    {
    public:
        RootSignature() = default;

        void build(const RootSignatureParams& params);

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
