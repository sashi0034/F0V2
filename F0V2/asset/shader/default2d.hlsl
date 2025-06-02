
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PSInput VS(float4 position : POSITION, float2 uv : TEXCOORD)
{
    PSInput result;
    result.position = position;
    result.uv = uv;
    return result;
}

Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    return float4(g_texture0.Sample(g_sampler0, input.uv));
}
