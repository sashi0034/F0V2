Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

cbuffer SceneState : register(b0)
{
    column_major float4x4 g_projectionMatrix;
    column_major float4x4 g_viewMatrix;
}

cbuffer ModelState : register(b1)
{
    column_major float4x4 g_worldMatrix;
}

cbuffer ModelMaterial : register(b2)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

cbuffer Skydome : register(b10)
{
    float4 g_topColor;
    float4 g_bottomColor;
    float g_sphereRadius;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float4 localPosition : POSITION;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.localPosition = position / g_sphereRadius;

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    const float4 finalColor = lerp(g_bottomColor, g_topColor, input.localPosition.y * 0.5 + 0.5);
    return finalColor;
}
