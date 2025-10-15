#pragma once
#include "TY/Shader.h"

namespace TY
{
    class TextureObject;

    class UnorderedStructuredBuffer;
}

namespace TY::detail
{
    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        TextureObject GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();

        ComputeShader GetStubCS();

        UnorderedStructuredBuffer GetEmptyStructuredBuffer();
    }
}
