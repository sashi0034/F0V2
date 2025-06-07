#include "pch.h"
#include "EnginePresetAsset.h"

#include "ShaderResourceTexture.h"
#include "TY/Image.h"
#include "TY/Shader.h"
#include "TY/Value2D.h"

using namespace TY;

using namespace TY::detail;

namespace
{
    struct EnginePresetAssetImpl
    {
        ShaderResourceTexture m_whiteTexture{};

        VertexShader m_stubVS{};

        PixelShader m_stubPS{};

        EnginePresetAssetImpl()
        {
            const Image whiteImage{Size{16, 16}, ColorU8{255}};
            m_whiteTexture = ShaderResourceTexture(whiteImage);

            m_stubVS = VertexShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "VS"}};

            m_stubPS = PixelShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "PS"}};
        }
    };

    std::shared_ptr<EnginePresetAssetImpl> s_enginePresetAsset{};

    void ensureInitialized()
    {
        if (not s_enginePresetAsset)
        {
            s_enginePresetAsset = std::make_shared<EnginePresetAssetImpl>();
        }
    }
}

namespace TY::detail
{
    void EnginePresetAsset::Init()
    {
        ensureInitialized();
    }

    void EnginePresetAsset::Shutdown()
    {
        s_enginePresetAsset.reset();
    }

    ShaderResourceTexture EnginePresetAsset::GetWhiteTexture()
    {
        ensureInitialized();
        return s_enginePresetAsset->m_whiteTexture;
    }

    VertexShader EnginePresetAsset::GetStubVS()
    {
        ensureInitialized();
        return s_enginePresetAsset->m_stubVS;
    }

    PixelShader EnginePresetAsset::GetStubPS()
    {
        ensureInitialized();
        return s_enginePresetAsset->m_stubPS;
    }
}
