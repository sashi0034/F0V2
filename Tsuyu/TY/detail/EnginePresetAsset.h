#pragma once
#include "TY/Shader.h"
#include "TY/ShaderResourceTexture.h"

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
