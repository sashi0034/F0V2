Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

cbuffer SceneState : register(b0)
{
    float4x4 g_projectionMatrix;
    float4x4 g_viewMatrix;
}

cbuffer ModelState : register(b1)
{
    float4x4 g_worldMatrix;
}

cbuffer ModelMaterial : register(b2)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

cbuffer ShadowMapDrawer : register(b4)
{
    float4x4 g_worldToShadowProjection;
}

struct PSInput
{
    float4 position : SV_POSITION;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);

    // result.position = mul(g_shadowView, result.position);
    // result.position = mul(g_shadowProjection, result.position);

    result.position = mul(g_worldToShadowProjection, result.position);

    // result.position = mul(g_viewMatrix, result.position);
    // result.position = mul(g_projectionMatrix, result.position);
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    return float4(0.5f, 0.5f, 0.5f, 1.0f);
}
