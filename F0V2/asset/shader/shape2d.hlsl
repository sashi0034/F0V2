Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

PSInput VS(float2 position : POSITION, float2 uv : TEXCOORD, float4 color : COLOR0)
{
    PSInput result;

    // TODO
    result.position = float4(position, 0.0, 1.0);
    result.position.x = result.position.x / 1920.0 * 2.0 - 1.0;
    result.position.y = result.position.y / 1080.0 * 2.0 - 1.0;

    result.uv = uv;
    result.color = color;
    return result;
}

// TODO: PS_Shape や PS_Texture などに分離
float4 PS(PSInput input) : SV_TARGET
{
    return input.color;
}
