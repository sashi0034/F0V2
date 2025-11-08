#define PI 3.14159265359

Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

// cbuffer SceneState : register(b0)
// {
//     column_major float4x4 g_projectionMatrix;
//     column_major float4x4 g_viewMatrix;
// }

cbuffer Shadertoy_b10 : register(b10)
{
    float2 g_outputResolution;
    float2 g_mousePosition;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    static const float2 pos[6] = {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    static const float2 uv[6] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0)
    };

    result.position = float4(pos[id], 0.0, 1.0);
    result.uv = uv[id];
    return result;
}

// -----------------------------------------------

uint HashUInt2(uint2 v)
{
    v ^= (v.yx >> 16);
    v *= 0x7feb352du;
    v ^= (v.yx >> 15);
    v *= 0x846ca68bu;
    v ^= (v.yx >> 16);
    return v.x ^ v.y;
}

float4 PS(PSInput input) : SV_TARGET
{
    // 基本情報
    uint laneIndex = WaveGetLaneIndex();
    uint laneCount = WaveGetLaneCount();

    uint2 pix = (uint2)input.position.xy;
    uint globalHash = HashUInt2(pix);

    // 各 Wave で先頭 Lane のみユニークカラーを決定
    float3 waveColor;
    if (laneIndex == 0)
    {
        // 擬似乱数で RGB 決定 (Waveごとに異なる色)
        uint h = globalHash;
        waveColor = float3(
            (h & 255u) / 255.0,
            ((h >> 8) & 255u) / 255.0,
            ((h >> 16) & 255u) / 255.0
        );
    }

    // Wave内で共有
    waveColor = WaveReadLaneFirst(waveColor);

    // 補助スレッド可視化
    bool helper = IsHelperLane();
    if (helper)
    {
        waveColor.rgb = frac(g_mousePosition.x);
    }

    return float4(waveColor, 1.0);
}
