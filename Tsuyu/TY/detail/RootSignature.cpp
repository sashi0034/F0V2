#include "pch.h"
#include "RootSignature.h"

#include "EngineRenderContext.h"
#include "TY/AssertObject.h"
#include "TY/System.h"
#include "TY/Utils.h"

namespace TY::detail
{
    RootSignature::RootSignature(const RootSignatureParams& params)
    {
        const auto& descriptorTable = params.descriptorTable;
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};

        // ディスクリプタテーブルの設定
        std::vector<D3D12_ROOT_PARAMETER> rootParameters{};
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> descriptorRanges{};
        int cbvOffset{};
        int srvOffset{};
        int uavOffset{};
        descriptorRanges.resize(descriptorTable.size());
        for (int i = 0; i < descriptorTable.size(); ++i)
        {
            if (descriptorTable[i].cbvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].cbvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                d.BaseShaderRegister = cbvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                cbvOffset += descriptorTable[i].cbvCount;
            }

            if (descriptorTable[i].srvCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].srvCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                d.BaseShaderRegister = srvOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                srvOffset += descriptorTable[i].srvCount;
            }

            if (descriptorTable[i].uavCount > 0)
            {
                D3D12_DESCRIPTOR_RANGE d{};
                d.NumDescriptors = descriptorTable[i].uavCount;
                d.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                d.BaseShaderRegister = uavOffset;
                d.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                descriptorRanges[i].push_back(d);
                uavOffset += descriptorTable[i].uavCount;
            }

            D3D12_ROOT_PARAMETER rootParameter = {};
            rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParameter.DescriptorTable.pDescriptorRanges = descriptorRanges[i].data();
            rootParameter.DescriptorTable.NumDescriptorRanges = descriptorRanges[i].size();
            rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

            rootParameters.push_back(rootParameter);
        }

        rootSignatureDesc.NumParameters = rootParameters.size();
        rootSignatureDesc.pParameters = rootParameters.data();
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        // -----------------------------------------------
        // サンプラーの設定
        D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 横繰り返し
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 縦繰り返し
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // 奥行繰り返し
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK; // ボーダーの時は黒
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT; // ニアレストネイバー
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX; // ミップマップ最大値
        samplerDesc.MinLOD = 0.0f; // ミップマップ最小値
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // ?
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダからアクセス

        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &samplerDesc;

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
            | EngineRenderContext::GetDevice()->CreateRootSignature(
                0,
                rootSignatureBlob->GetBufferPointer(),
                rootSignatureBlob->GetBufferSize(),
                IID_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf()));
        rootSignatureBlob->Release();
    }
}
