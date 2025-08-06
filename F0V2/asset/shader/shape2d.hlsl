Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer ShapeDraw : register(b0)
{
    row_major float2x4 g_transform;
}

float4 transform2D(float2 pos, float2x4 t)
{
    return float4((t._13_14 + (pos.x * t._11_12) + (pos.y * t._21_22)), t._23_24);
}

PSInput VS(float2 position : POSITION, float2 uv : TEXCOORD, float4 color : COLOR0)
{
    PSInput result;

    result.position = transform2D(position, g_transform);
    result.uv = uv;
    result.color = color;

    return result;
}

// TODO: PS_Shape や PS_Texture などに分離
float4 PS(PSInput input) : SV_TARGET
{
    return input.color;
}
