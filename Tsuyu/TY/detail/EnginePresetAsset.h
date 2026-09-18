#pragma once
#include "TY/Shader.h"

namespace TY
{
    class TextureHandle;

    class UnorderedStructuredBufferObject;
}

namespace TY::detail
{
    class ComputePipelineState;

    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        TextureHandle GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();

        ComputeShader GetStubCS();

        ComputePipelineState GetGenerateMipsPSO();

        UnorderedStructuredBufferObject GetEmptyStructuredBuffer();
    }
}
