// NOTE: 上下左右にタイリングされるので、境界が繋がるように frac ベースで模様を作ること

#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

SamplerState g_sampler0 : register(s0);

cbuffer CourseDynamicTexture_b10 : register(b10)
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

float hash12_(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

/// @brief タイルの境界からの距離 (0: 境界上, 0.5: タイル中心)
float tileBorderDistance_(float2 uv, float2 tiling)
{
    const float2 st = abs(0.5 - frac(uv * tiling));
    return 0.5 - max(st.x, st.y);
}

// -----------------------------------------------

float4 PS_RoadTop(PSInput input) : SV_Target
{
    const float2 uv = input.uv;

    // 金属パネルの継ぎ目
    static const float2 k_tiling = float2(4, 4);
    const float border = tileBorderDistance_(uv, k_tiling);
    const float seam = 1.0 - smoothstep(0.0, 0.03, border);

    // パネルごとの明度のばらつき
    const float2 tileId = floor(uv * k_tiling);
    const float tileNoise = hash12_(tileId) * 0.12;

    float3 rgb = float3(0.22, 0.23, 0.27) + tileNoise;
    rgb = lerp(rgb, float3(0.09, 0.10, 0.13), seam);

    // 進行方向に流れるエネルギーの脈動
    const float pulse = frac(uv.y - g_time * 0.15);
    const float glow = pow(saturate(1.0 - abs(pulse - 0.5) * 3.0), 6.0);
    rgb += float3(0.05, 0.35, 0.55) * glow * (1.0 - seam);

    return float4(sRGB2L_(rgb), 1.0);
}

float4 PS_RoadBottom(PSInput input) : SV_Target
{
    const float2 uv = input.uv;

    // 裏面は暗く、回路のようなグリッドを光らせる
    static const float2 k_tiling = float2(8, 8);
    const float border = tileBorderDistance_(uv, k_tiling);
    const float grid = 1.0 - smoothstep(0.0, 0.02, border);

    float3 rgb = float3(0.05, 0.05, 0.07);

    const float blink = 0.5 + 0.5 * sin(g_time * 1.5 + hash12_(floor(uv * k_tiling)) * TWO_PI);
    rgb += float3(0.10, 0.35, 0.45) * grid * (0.4 + 0.6 * blink);

    return float4(sRGB2L_(rgb), 1.0);
}

float4 PS_RoadSide(PSInput input) : SV_Target
{
    const float2 uv = input.uv;

    // 側面は流れる斜めストライプ
    // NOTE: uv.x + uv.y の周期を整数にしておかないとタイリングで繋がらない
    const float stripe = frac((uv.x + uv.y) * 6.0 - g_time * 0.4);
    const float band = smoothstep(0.45, 0.5, abs(stripe - 0.5));

    float3 rgb = lerp(float3(0.16, 0.17, 0.20), float3(0.55, 0.30, 0.03), band);

    // 上端と下端を暗く落として厚みを出す
    const float edge = smoothstep(0.0, 0.25, min(uv.y, 1.0 - uv.y));
    rgb *= 0.45 + 0.55 * edge;

    return float4(sRGB2L_(rgb), 1.0);
}
