#pragma once
#include "TY/Shader.h"

namespace TY
{
    class ShaderResourceTexture;

    class StructuredBufferTransfer;
}

namespace TY::detail
{
    namespace EnginePresetAsset
    {
        void Init();

        void Shutdown();

        ShaderResourceTexture GetWhiteTexture();

        VertexShader GetStubVS();

        PixelShader GetStubPS();

        ComputeShader GetStubCS();

        StructuredBufferTransfer GetEmptyStructuredBuffer();
    }
}
