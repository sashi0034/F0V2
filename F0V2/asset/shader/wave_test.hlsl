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

struct SdfAndMat
{
    float sdf;
    float mat; // TODO
};

SdfAndMat emptySdfAndMat()
{
    SdfAndMat result;
    result.sdf = 1e10;
    result.mat = 0.0;
    return result;
}

float2 rotate2d(float2 p, float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float2(
        p.x * c - p.y * s,
        p.x * s + p.y * c
    );
}

float sdfSphere(float3 p, float r)
{
    return length(p) - r;
}

// float sdfBox(float3 p, float3 b)
// {
//     float3 d = abs(p) - b;
//     return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
// }
//
// static float g_morphRate = 0;
//
// float sdfMorphSphereBox(float3 p, float r, float3 b, float rate)
// {
//     float dSphere = sdfSphere(p, r);
//     float dBox = sdfBox(p, b);
//     return lerp(dSphere, dBox, rate);
// }
//
// float smoothMin(float a, float b, float k)
// {
//     float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
//     return lerp(b, a, h) - k * h * (1.0 - h);
// }

SdfAndMat scanSdf(float3 pos)
{
    SdfAndMat result = emptySdfAndMat();

    float dSphere = sdfSphere(pos - float3(0, 0, 0), 0.75);
    if (dSphere < result.sdf)
    {
        result.sdf = dSphere;
        result.mat = 1.0;
    }

    return result;
}

float3 scanNormal(float3 pos)
{
    float h = 1e-4;
    float3 n;
    n.x = scanSdf(pos + float3(h, 0, 0)).sdf - scanSdf(pos - float3(h, 0, 0)).sdf;
    n.y = scanSdf(pos + float3(0, h, 0)).sdf - scanSdf(pos - float3(0, h, 0)).sdf;
    n.z = scanSdf(pos + float3(0, 0, h)).sdf - scanSdf(pos - float3(0, 0, h)).sdf;
    return normalize(n);
}

struct RaycastResult
{
    float3 pos;
    SdfAndMat d;
};

#define EPS 1e-4

#define MAX_DIST 1000.0

#define MAX_RAYMARCH 64

RaycastResult scanRaycast(float3 pos, float3 dir)
{
    RaycastResult result;
    result.pos = float3(0, 0, 0);
    result.d = emptySdfAndMat();

    float t = 0;
    for (int i = 0; i < MAX_RAYMARCH; ++i)
    {
        float3 p = pos + dir * t;
        SdfAndMat d = scanSdf(p);
        if (d.sdf < EPS)
        {
            result.pos = p;
            result.d = d;
            break;
        }

        t += d.sdf;
    }

    return result;
}

// Bayer 4x4 Dither Matrix (0〜15)
static const float Dither4x4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

// -----------------------------------------------

float4 PS(PSInput input) : SV_TARGET
{
    // ウェーブ情報
    uint lane = WaveGetLaneIndex();
    uint count = WaveGetLaneCount();

    // 2x2 Quad 内の座標（0 or 1）
    uint2 pix = (uint2)input.position.xy; // SV_Position.xy を整数化
    uint2 qofs = pix & 1; // (x&1, y&1)

    // ヘルパー判定（SM 6.6+）
    bool helper = IsHelperLane();

    // タイル感を見るための簡易色
    float3 rgb = float3(
        (lane % 32) / 31.0, // ウェーブ内順序
        float((pix.x & 7u) | ((pix.y & 7u) << 3)) / 127.0, // 8x8 の折返し感
        (qofs.x + 2 * qofs.y) / 3.0
    );

    if (helper) rgb *= 0.3;
    return float4(rgb, 1);
}
