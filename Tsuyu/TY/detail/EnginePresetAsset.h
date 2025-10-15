#pragma once
#include "TY/Shader.h"

namespace TY
{
    class TextureHandle;

    class UnorderedStructuredBuffer;
}

namespace TY::detail
{
    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        TextureHandle GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();

        ComputeShader GetStubCS();

        UnorderedStructuredBuffer GetEmptyStructuredBuffer();
    }
}
