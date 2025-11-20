Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    // float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer SceneState : register(b0)
{
    column_major float4x4 g_projectionMatrix;
    column_major float4x4 g_viewMatrix;
}

cbuffer ImmediateDrawer : register(b1)
{
    row_major float2x4 g_transform;
    float4 g_colorMul;
    float4 g_colorAdd;
}

PSInput VS(float3 position : POSITION, float4 color : COLOR0)
{
    PSInput result;

    result.position = float4(position, 1.0);
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.color = color * g_colorMul;

    return result;
}

float4 PS_Shape(PSInput input) : SV_TARGET
{
    return input.color + g_colorAdd;
}
