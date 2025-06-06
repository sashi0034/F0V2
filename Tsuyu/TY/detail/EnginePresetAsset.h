#pragma once
#include "ShaderResourceTexture.h"
#include "TY/Shader.h"

namespace TY::detail
{
    class EnginePresetAsset_impl
    {
    public:
        void Init() const;

        void Destroy() const;

        ShaderResourceTexture GetWhiteTexture() const;

        VertexShader GetStubVS() const;

        PixelShader GetStubPS() const;
    };

    inline constexpr auto EnginePresetAsset = EnginePresetAsset_impl{};
}
