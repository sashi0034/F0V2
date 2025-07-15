Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

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

static const float INT8_MAX_F = 127.0f;
static const float INT16_MAX_F = 32767.0f;
static const float INT32_MAX_F = 2147483647.0f;

float4 PS(PSInput input) : SV_TARGET
{
    const float r32_float = float4(g_texture0.Sample(g_sampler0, input.uv)).x;
    const float r32_float_abs = abs(r32_float);

    float4 result;
    result.a = 1.0f;

    const float clearValue = 1.0f;
    if (r32_float == clearValue)
    {
        result.rgb = float3(0.0f, 0.0f, 0.0f);
    }
    else
    {
        result.r = r32_float > 0.0f ? 1.0f : 0.0f;

        result.g = r32_float_abs / INT8_MAX_F;

        result.b = r32_float_abs;
    }

    return result;
}
