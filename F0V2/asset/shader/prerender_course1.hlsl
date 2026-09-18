// NOTE: 上下左右にタイリングされるので、境界が繋がるように frac ベースで模様を作ること

#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

SamplerState g_sampler0 : register(s0);

cbuffer CourseRenderTexture_b10 : register(b10)
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
float3 sRGB2L(float3 srgb)
{
    return srgb * (srgb * (srgb * 0.305306011 + 0.682171111) + 0.012522878);
}

float hash12(float2 p)
{
    float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

// タイリング可能な値ノイズ (period には境界を繋げるために整数を渡すこと)
float tilingNoise(float2 p, float period)
{
    const float2 cell = floor(p);

    float2 t = frac(p);
    t = t * t * (3.0 - 2.0 * t);

    const float2 a = cell - floor(cell / period) * period;
    float2 b = a + 1.0;
    b -= floor(b / period) * period;

    return lerp(
        lerp(hash12(a), hash12(float2(b.x, a.y)), t.x),
        lerp(hash12(float2(a.x, b.y)), hash12(b), t.x),
        t.y);
}

// tilingNoise_ を重ねた星雲のゆらぎ
float nebulaFbm(float2 uv)
{
    static const int k_octaveCount = 4;

    float sum = 0.0;
    float weight = 0.533333;
    float frequency = 4.0;

    [unroll]
    for (int i = 0; i < k_octaveCount; ++i)
    {
        sum += weight * tilingNoise(uv * frequency, frequency);
        frequency *= 2.0;
        weight *= 0.5;
    }

    return sum;
}

// 2 つの距離場をなめらかに結合する (k は融合の広がり)
float sdfSmoothUnion(float a, float b, float k)
{
    const float h = saturate(0.5 + 0.5 * (b - a) / k);
    return lerp(b, a, h) - k * h * (1.0 - h);
}

// 上下端をまたいでも繋がるよう、y 方向の距離を周期化する
float cyclicDeltaY(float y, float centerY)
{
    return frac(y - centerY + 0.5) - 0.5;
}

// -----------------------------------------------

float4 PS_RoadTop(PSInput input) : SV_Target
{
    const float2 uv = input.uv;
    const float roadX = abs(uv.x * 2.0 - 1.0); // 0: 中央, 1: 両端

    // ワープだけでなくガス自体も流すことで、止まって見えないようにする
    const float2 drift = float2(g_time * 0.05, -g_time * 0.026);
    const float2 warp = float2(nebulaFbm(uv + drift), nebulaFbm(uv + float2(0.37, 0.61) - drift));
    const float2 p = uv + drift * 0.65 + 0.42 * (warp - 0.5);

    const float cloud = nebulaFbm(p + float2(0.13, 0.29));
    const float detail = nebulaFbm(p * 2.0 - drift);

    const float body = smoothstep(0.28, 0.76, cloud); // ガスの本体
    const float filament = pow(saturate(1.0 - abs(cloud - 0.53) * 10.0), 3.0); // ガスの輪郭に走る筋
    const float dust = smoothstep(0.43, 0.68, detail); // 手前を横切る暗い塵
    const float hue = nebulaFbm(uv + float2(0.71, 0.19) - drift * 0.5);

    // ワープしたガスに沿って流れる 6 秒周期の光の波
    // NOTE: uv.y の係数を整数にしておかないとタイリングで繋がらない
    float lightWave = 0.5 + 0.5 * sin(TWO_PI * (uv.y * 2.0 + warp.x * 1.5 - g_time / 6.0));
    lightWave = lightWave * lightWave * lightWave;

    // 真珠色をベースに、光の波の分の明度を残しておく
    float3 rgb = float3(0.68, 0.69, 0.70);

    const float3 gas = lerp(float3(0.18, 0.19, 0.20), float3(0.24, 0.22, 0.17), smoothstep(0.3, 0.7, hue));
    rgb += gas * body * 0.85;
    rgb += float3(0.16, 0.15, 0.10) * filament * (0.25 + 0.75 * detail);
    rgb *= 1.0 - 0.15 * dust;
    rgb += float3(0.18, 0.16, 0.12) * lightWave * (0.25 * body + 0.75 * filament);
    rgb += float3(0.16, 0.16, 0.15) * pow(body, 3.0) * 0.35;

    // 中央には静止円を等間隔に並べ、同じ個数・同じ間隔の移動円が同じ軌道を進む。
    // 円の距離場を smooth union で結合することで、接近時にメタボール状につながる。
    static const float k_centerCircleCount = 20.0;
    static const float k_circleRadius = 0.005;
    static const float k_circleFusion = 0.020; // 融合の広がり
    const float centerCell = floor(uv.y * k_centerCircleCount);
    const float movingPhase = frac(g_time * 0.15 * k_centerCircleCount); // セル 1 個分を進む位相

    float metaballSdf = 1.0;

    [unroll]
    for (int i = -1; i <= 1; ++i)
    {
        const float staticY = (centerCell + float(i) + 0.5) / k_centerCircleCount;
        const float movingY = staticY + movingPhase / k_centerCircleCount;

        const float2 staticDelta = float2(uv.x - 0.5, cyclicDeltaY(uv.y, staticY));
        const float2 movingDelta = float2(uv.x - 0.5, cyclicDeltaY(uv.y, movingY));

        metaballSdf = sdfSmoothUnion(metaballSdf, length(staticDelta) - k_circleRadius, k_circleFusion);
        metaballSdf = sdfSmoothUnion(metaballSdf, length(movingDelta) - k_circleRadius, k_circleFusion);
    }

    // 距離場の勾配はほぼ一定なので、静止円も移動円も同じ幅でくっきり縁取りされる
    const float sdfAa = max(fwidth(metaballSdf), 1e-5);
    const float centerMetaball = 1.0 - smoothstep(-sdfAa, sdfAa, metaballSdf);

    const float3 platinumGold = float3(0.90, 0.85, 0.15);

    // 両端線もここで描き、gbuffer 側には完成した一枚を渡す。
    const float sideLine = 1.0 - smoothstep(0.012, 0.025, abs(roadX - 0.91));
    const float shoulder = smoothstep(0.80, 1.0, roadX);
    const float sideLinePulse = 0.90 + 0.10 * sin(TWO_PI * (uv.y * 4.0 - g_time * 0.25));

    rgb *= lerp(1.0, 0.62, shoulder);
    rgb = lerp(rgb, platinumGold, centerMetaball);
    rgb = lerp(rgb, platinumGold * sideLinePulse, sideLine);

    return float4(sRGB2L(saturate(rgb)), 1.0);
}

float4 PS_RoadBottom(PSInput input) : SV_Target
{
    // 裏面はほとんど見えないので、ゆっくり流れる濃淡だけ
    const float cloud = nebulaFbm(input.uv + float2(g_time * 0.002, 0.0));

    const float3 rgb = lerp(float3(0.32, 0.33, 0.34), float3(0.52, 0.51, 0.48), cloud);

    return float4(sRGB2L(rgb), 1.0);
}

float4 PS_RoadSide(PSInput input) : SV_Target
{
    const float2 uv = input.uv;

    // 側面は横に流れる 1 本のリボン
    // NOTE: uv.x, uv.y の係数を整数にしておかないとタイリングで繋がらない
    const float wave = sin(TWO_PI * (uv.x * 2.0 - g_time * 0.06));
    const float ribbon = pow(saturate(1.0 - abs(sin(TWO_PI * uv.y) + wave * 0.25)), 8.0);

    const float3 rgb = float3(0.53, 0.54, 0.55) + float3(0.25, 0.23, 0.18) * ribbon;

    return float4(sRGB2L(rgb), 1.0);
}
