#include "pch.h"
#include "EnginePresetAsset.h"

#include "ShaderResourceTexture.h"
#include "ZG/Image.h"
#include "ZG/Shader.h"
#include "ZG/Value2D.h"

using namespace ZG;

using namespace ZG::detail;

namespace
{
    struct Impl
    {
        ShaderResourceTexture m_whiteTexture;

        VertexShader m_stubVS;

        PixelShader m_stubPS;

        Impl()
        {
            const Image whiteImage{Size{16, 16}, ColorU8{255}};
            m_whiteTexture = ShaderResourceTexture(whiteImage);

            m_stubVS = VertexShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "VS"}};

            m_stubPS = PixelShader{ShaderParams{.filename = "engine/stub.hlsl", .entryPoint = "PS"}};
        }
    };

    std::shared_ptr<Impl> s_enginePresetAsset{};
}

namespace ZG::detail
{
    void EnginePresetAsset_impl::Init() const
    {
        s_enginePresetAsset = std::make_shared<Impl>();
    }

    void EnginePresetAsset_impl::Destroy() const
    {
        s_enginePresetAsset.reset();
    }

    ShaderResourceTexture EnginePresetAsset_impl::GetWhiteTexture() const
    {
        return s_enginePresetAsset->m_whiteTexture;
    }

    VertexShader EnginePresetAsset_impl::GetStubVS() const
    {
        return s_enginePresetAsset->m_stubVS;
    }

    PixelShader EnginePresetAsset_impl::GetStubPS() const
    {
        return s_enginePresetAsset->m_stubPS;
    }
}
