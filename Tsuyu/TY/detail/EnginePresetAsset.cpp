#include "pch.h"
#include "EnginePresetAsset.h"

#include "ShaderResourceTexture.h"
#include "TY/Image.h"
#include "TY/Shader.h"
#include "TY/Value2D.h"

using namespace TY;
using namespace TY::detail;

struct EnginePresetAssetImpl
{
    bool m_initialized = false;

    ShaderResourceTexture m_whiteTexture{};

    VertexShader m_stubVS{};

    PixelShader m_stubPS{};

    void Init()
    {
        const Image whiteImage{Size{16, 16}, ColorU8{255}};
        m_whiteTexture = ShaderResourceTexture(whiteImage);

        m_stubVS = VertexShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "VS"}};

        m_stubPS = PixelShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "PS"}};

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
}
