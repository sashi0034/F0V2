#pragma once
#include "ShaderResourceTexture.h"
#include "TY/Shader.h"

namespace TY::detail
{
    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        ShaderResourceTexture GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();
    };
}
