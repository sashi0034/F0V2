#pragma once
#include "TY/Shader.h"

namespace TY
{
    class TextureResource;

    class UnorderedStructuredBuffer;
}

namespace TY::detail
{
    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        TextureResource GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();

        ComputeShader GetStubCS();

        UnorderedStructuredBuffer GetEmptyStructuredBuffer();
    }
}
