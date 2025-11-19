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
    float d = sdfHeart(input.uv);

    float3 rgb = float3(1, 1, 1);

    rgb.gb = step(d, abs(sin(d * 8 - g_time)));

    return float4(sRGB2L_(rgb), 1.0);
}
