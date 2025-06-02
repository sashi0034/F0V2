#include "basic.hlsli"

cbuffer SceneState : register(b0)
{
    float4x4 g_worldMat;
    float4x4 g_viewMat;
    float4x4 g_projectionMat;
}

PSInput VS(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMat, position);
    result.position = mul(g_viewMat, result.position);
    result.position = mul(g_projectionMat, result.position);

    result.uv = uv;
    return result;
}
