#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

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

float circle(float2 uv)
{
    float2 p = uv - float2(0.5, 0.5);
    float d = length(p);
    return smoothstep(0.3, 0.28, d);
}

float3 hsv2rgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, saturate(p - K.xxx), c.y);
}

float distortUV(float2 uv)
{
    float x = 2.0 * uv.y + sin(g_time * 1.0);
    return sin(g_time * 2.0) * 0.1 *
        sin(5.0 * x) * (-(x - 1.0) * (x - 1.0) + 1.0);
}

float4 PS(PSInput input) : SV_Target
{
    float2 uv = input.uv;

    float distort = distortUV(uv);
    uv.x += distort;

    // カラフル背景
    float hue = frac(uv.x + uv.y + g_time * 0.3);
    float3 bg = hsv2rgb(float3(hue, 1.0, 1.0));

    // RGB ずらし
    float r = circle(uv + float2(0, -distort) * 0.3);
    float g = circle(uv + float2(0, distort) * 0.3);
    float b = circle(uv + float2(distort, 0) * 0.3);

    float3 outCol = float3(r, g, b);
    if (all(outCol == 0.0))
    {
        outCol = bg;
    }

    return float4(sRGB2L_(outCol), 1.0);
}
