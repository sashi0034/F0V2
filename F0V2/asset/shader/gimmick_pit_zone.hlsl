#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

Texture2D<float4> g_texture0 : register(t10);

SamplerState g_sampler0 : register(s0);

cbuffer Gimmick_b10 : register(b10)
{
    float g_time;
}

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    static const float2 k_pos[6] = {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    static const float2 k_uv[6] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0)
    };

    result.pos = float4(k_pos[id], 0.0, 1.0);
    result.uv = k_uv[id];
    return result;
}

// -----------------------------------------------

// from sRGB to Linear (approximate)
float3 sRGB2L_(float3 srgb)
{
    return srgb * (srgb * (srgb * 0.305306011 + 0.682171111) + 0.012522878);
}

float sdfHeart(float2 st)
{
    st = (st - float2(0.5, 0.38)) * float2(2.1, 2.8);

    return pow(st.x, 2) + pow(st.y - sqrt(abs(st.x)), 2);
}

float4 PS(PSInput input) : SV_Target
{
    float2 uv = input.uv;
    uv.y -= g_time * 0.1f;
    uv = frac(uv);

    float2 fracUV = uv.x;
    fracUV.y = abs(0.5f - frac(uv.y));

    const float3 tlC = float3(0.99, 0.4, 0.57);
    const float3 trC = float3(1, 0.6, 0.07);
    const float3 brC = float3(1, 0.4, 0);
    const float3 blC = tlC;

    float3 rgb =
        tlC * (1 - fracUV.x) * (1 - fracUV.y) +
        trC * fracUV.x * (1 - fracUV.y) +
        brC * fracUV.x * fracUV.y +
        blC * (1 - fracUV.x) * fracUV.y;

    const float sd1 = sdfHeart(uv);
    const float f = 1.0 - step(sd1, abs(sin(sd1 * 12 - g_time * 4)));
    rgb += f * 0.3;
    rgb.rb *= 1.0 + 1.0 * f;

    const float2 uv2 = frac(uv * float2(4, 4) + float2(0, -g_time * 1.0));
    const float sd2 = sdfHeart(uv2);
    const float f2 = 0.3f + 0.5 * min(f, step(sd2, abs(sin(sd2 * 8 - g_time * 8))));
    rgb.g *= f2;

    return float4(sRGB2L_(rgb), 1.0);
}
