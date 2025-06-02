#include "model.hlsli"

cbuffer SceneState : register(b0)
{
    float4x4 g_worldMat;
    float4x4 g_viewMat;
    float4x4 g_projectionMat;
}

cbuffer ModelMaterial : register(b1)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMat, position);
    result.position = mul(g_viewMat, result.position);
    result.position = mul(g_projectionMat, result.position);

    result.color = g_diffuse;

    result.uv = uv;
    return result;
}
