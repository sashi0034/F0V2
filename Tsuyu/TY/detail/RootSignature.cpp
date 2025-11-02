#include "pch.h"
#include "RootSignature.h"

#include "RenderContext_singleton.h"
#include "TY/AssertObject.h"
#include "TY/Logger.h"
#include "TY/System.h"
#include "TY/Utils.h"

using namespace TY;
using namespace TY::detail;

namespace
{
    D3D12_TEXTURE_ADDRESS_MODE getAddressMode(GraphicsAddressMode mode)
    {
        switch (mode)
        {
        case GraphicsAddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case GraphicsAddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case GraphicsAddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case GraphicsAddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case GraphicsAddressMode::MirrorOnce: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        }

        assert(false);
        return {};
    }

    enum class FilterGroup : uint8_t
    {
        Normal,
        Comparison,
    };

    D3D12_FILTER getFilterMode(GraphicsFilterMode filter, FilterGroup group)
    {
        switch (filter)
        {
        case GraphicsFilterMode::Nearest:
            switch (group)
            {
            case FilterGroup::Normal: return D3D12_FILTER_MIN_MAG_MIP_POINT;
            case FilterGroup::Comparison: return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            }
        case GraphicsFilterMode::Linear:
            switch (group)
            {
            case FilterGroup::Normal: return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            case FilterGroup::Comparison: return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            }
        case GraphicsFilterMode::Aniso:
            switch (group)
            {
            case FilterGroup::Normal: return D3D12_FILTER_ANISOTROPIC;
            case FilterGroup::Comparison: return D3D12_FILTER_COMPARISON_ANISOTROPIC;
            }
        }

        assert(false);
        return {};
    }

    D3D12_COMPARISON_FUNC getComparisonFunction(GraphicsComparisonFunction comparison)
    {
        switch (comparison)
        {
        case GraphicsComparisonFunction::Never: return D3D12_COMPARISON_FUNC_NEVER;
        case GraphicsComparisonFunction::Less: return D3D12_COMPARISON_FUNC_LESS;
        case GraphicsComparisonFunction::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
        case GraphicsComparisonFunction::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case GraphicsComparisonFunction::Greater: return D3D12_COMPARISON_FUNC_GREATER;
        case GraphicsComparisonFunction::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case GraphicsComparisonFunction::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case GraphicsComparisonFunction::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
        }

        assert(false);
        return {};
    }
}

namespace TY::detail
{
    RootSignature::RootSignature(const RootSignatureParams& params)
    {
        const auto& descriptorTable = params.descriptorTable;
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

        // ディスクリプタテーブルの設定
        std::span explicitRegisterStarts = params.explicitRegisterStarts;
        std::vector<D3D12_ROOT_PARAMETER> rootParameters{};
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges{};
        int cbvOffset{};
        int srvOffset{};
        int uavOffset{};
        descriptorRanges.resize(descriptorTable.size());
        for (int tableIndex = 0; tableIndex < descriptorTable.size(); ++tableIndex)
        {
            if (not explicitRegisterStarts.empty() && tableIndex == explicitRegisterStarts[0].descriptorTableIndex)
            {
                // 明示的なレジスタ開始番号が指定されている場合、オフセットを変更
                if (explicitRegisterStarts[0].cbvStart >= cbvOffset)
                {
                    cbvOffset = explicitRegisterStarts[0].cbvStart;
                }
                else
                {
                    LogError(std::format(
                        "RootSignature: cbvOffset is greater than explicit cbvStart in table {}.", tableIndex));
                }

                if (explicitRegisterStarts[0].srvStart >= srvOffset)
                {
                    srvOffset = explicitRegisterStarts[0].srvStart;
                }
                else
                {
                    LogError(std::format(
                        "RootSignature: srvOffset is greater than explicit srvStart in table {}.", tableIndex));
                }

                if (explicitRegisterStarts[0].uavStart >= uavOffset)
                {
                    uavOffset = explicitRegisterStarts[0].uavStart;
                }
                else
                {
                    LogError(std::format(
                        "RootSignature: uavOffset is greater than explicit uavStart in table {}.", tableIndex));
                }

                explicitRegisterStarts = explicitRegisterStarts.subspan(1);
            }

            // CBV 設定
            if (descriptorTable[tableIndex].cbvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[tableIndex].cbvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                d.BaseShaderRegister = cbvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[tableIndex].push_back(d);
                cbvOffset += descriptorTable[tableIndex].cbvCount;
            }

            // SRV 設定
            if (descriptorTable[tableIndex].srvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[tableIndex].srvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                d.BaseShaderRegister = srvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[tableIndex].push_back(d);
                srvOffset += descriptorTable[tableIndex].srvCount;
            }

            // UAV 設定
            if (descriptorTable[tableIndex].uavCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[tableIndex].uavCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                d.BaseShaderRegister = uavOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[tableIndex].push_back(d);
                uavOffset += descriptorTable[tableIndex].uavCount;
            }

            // ルートパラメータの設定
            D3D12_ROOT_PARAMETER rootParameter = {};
            rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameter.DescriptorTable.pDescriptorRanges = descriptorRanges[tableIndex].data();
            rootParameter.DescriptorTable.NumDescriptorRanges = descriptorRanges[tableIndex].size();
            rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            rootParameters.push_back(rootParameter);
        }

        rootSignatureDesc.NumParameters = rootParameters.size();
        rootSignatureDesc.pParameters = rootParameters.data();
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // -----------------------------------------------
        // サンプラーの設定
        Array<D3D12_STATIC_SAMPLER_DESC> samplerDescList{};
        samplerDescList.reserve(params.samplers.size());
        for (int i = 0; i < params.samplers.size(); ++i)
        {
            const auto& sampler = params.samplers[i];

            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.AddressU = getAddressMode(sampler.addressU);
            samplerDesc.AddressV = getAddressMode(sampler.addressV);
            samplerDesc.AddressW = getAddressMode(sampler.addressW);

            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーの時は透明

            const bool hasComparisonFunction = sampler.comparison != GraphicsComparisonFunction::Never;
            const auto filterGroup = hasComparisonFunction ? FilterGroup::Comparison : FilterGroup::Normal;
            samplerDesc.Filter = getFilterMode(sampler.filter, filterGroup);

            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
            samplerDesc.MinLOD = 0.0f; // ミップマップ最小値

            samplerDesc.MaxAnisotropy = sampler.maxAnisotropy;

            samplerDesc.ComparisonFunc = getComparisonFunction(sampler.comparison);

            samplerDesc.ShaderRegister = i;

            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            samplerDescList.push_back(samplerDesc);
        }

        rootSignatureDesc.NumStaticSamplers = static_cast<UINT>(samplerDescList.size());
        rootSignatureDesc.pStaticSamplers = &samplerDescList[0];

        // -----------------------------------------------
        // ルートシグネチャの作成
        ComPtr<ID3D10Blob> rootSignatureBlob{};
        ComPtr<ID3DBlob> errorBlob = nullptr;
        D3D12SerializeRootSignature(
            &rootSignatureDesc,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &rootSignatureBlob,
            &errorBlob);
        if (errorBlob)
        {
            System::ModalError(StringifyBlob(errorBlob.Get()));
            throw std::runtime_error("failed to serialize root signature");
        }

        AssertWin32{"failed to create root signature"sv}
            | RenderContext_singleton::GetDevice()->CreateRootSignature(
                0,
                rootSignatureBlob->GetBufferPointer(),
                rootSignatureBlob->GetBufferSize(),
                IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));
        rootSignatureBlob->Release();
    }
}
