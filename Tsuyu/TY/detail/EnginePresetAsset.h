#pragma once
#include "TY/Shader.h"

namespace TY
{
    class TextureHandle;

    class UnorderedStructuredBufferObject;
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

        UnorderedStructuredBufferObject GetEmptyStructuredBuffer();
    }
}
