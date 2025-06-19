#include "pch.h"
#include "EnginePresetAsset.h"

#include "TY/Image.h"
#include "TY/Shader.h"
#include "TY/ShaderResourceTexture.h"
#include "TY/StructuredBufferUploader.h"
#include "TY/Value2D.h"

using namespace TY;
using namespace TY::detail;

struct EnginePresetAssetImpl
{
    bool m_initialized = false;

    ShaderResourceTexture m_whiteTexture{};

    VertexShader m_stubVS{};

    PixelShader m_stubPS{};

    ComputeShader m_stubCS{};

    StructuredBufferTransfer m_emptyStructuredBuffer{};

    void Init()
    {
        const Image whiteImage{Size{16, 16}, ColorU8{255}};
        m_whiteTexture = ShaderResourceTexture(whiteImage);

        m_stubVS = VertexShader{ShaderParams::VS("engine/graphics_stub.hlsl")};

        m_stubPS = PixelShader{ShaderParams::PS("engine/graphics_stub.hlsl")};

        m_stubCS = ComputeShader{ShaderParams::CS("engine/compute_stub.hlsl")};

        m_emptyStructuredBuffer = StructuredBufferTransfer(
            StructuredBufferTransferParams{
                .elementCount = 1,
                .elementStride = sizeof(uint8_t)
            }
        );

        m_initialized = true;
    }
};

namespace
{
    EnginePresetAssetImpl s_enginePresetAsset{};
}

namespace TY::detail
{
    void EnginePresetAsset::Init()
    {
        s_enginePresetAsset.Init();
    }

    void EnginePresetAsset::Shutdown()
    {
        s_enginePresetAsset = {};
    }

    ShaderResourceTexture EnginePresetAsset::GetWhiteTexture()
    {
        assert(s_enginePresetAsset.m_initialized);
        return s_enginePresetAsset.m_whiteTexture;
    }

    VertexShader EnginePresetAsset::GetStubVS()
    {
        assert(s_enginePresetAsset.m_initialized);
        return s_enginePresetAsset.m_stubVS;
    }

    PixelShader EnginePresetAsset::GetStubPS()
    {
        assert(s_enginePresetAsset.m_initialized);
        return s_enginePresetAsset.m_stubPS;
    }

    ComputeShader EnginePresetAsset::GetStubCS()
    {
        assert(s_enginePresetAsset.m_initialized);
        return s_enginePresetAsset.m_stubCS;
    }

    StructuredBufferTransfer EnginePresetAsset::GetEmptyStructuredBuffer()
    {
        assert(s_enginePresetAsset.m_initialized);
        return s_enginePresetAsset.m_emptyStructuredBuffer;
    }
}
