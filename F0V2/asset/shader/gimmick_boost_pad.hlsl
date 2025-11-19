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

float3 sRGB2L_(float3 srgb)
{
    return srgb * (srgb * (srgb * 0.305306011 + 0.682171111) + 0.012522878);
}

float4 PS(PSInput input) : SV_Target
{
    // 参考: https://docs.google.com/presentation/d/1NMhx4HWuNZsjNRRlaFOu2ysjo04NgcpFlEhzodE8Rlg/edit?slide=id.g364407fc99_0_100#slide=id.g364407fc99_0_100

    float2 uv = input.uv;
    uv.y -= g_time * 0.5;
    uv.y = frac(uv.y);

    const int tileId = int(floor(uv.x * 2)) + int(floor(uv.y * 2)) * 2;

    float2 st = 0.5 - frac(uv * 2);

    float a = atan2(st.y, st.x);

    const float phase = a * 2.5 + g_time * (1.0 + 3.0 * tileId);
    float r = length(st);
    float d = min(abs(cos(phase)) + 0.4, abs(sin(phase)) + 1.1) * 0.32;

    float3 color = lerp(float3(1, 0.87, 0.13), float3(1, 0.47, 0.03), input.uv.y);

    float petal = step(r, d);
    color = lerp(color, lerp(float3(1, 0.3, 1), float3(0.65, 0.03, 0.93), r * 2.5), petal);

    float cap = step(distance(0, st), 0.07);
    color = lerp(color, float3(0.99, 0.78, 0), cap);

    return float4(sRGB2L_(color), 1.0);
}
