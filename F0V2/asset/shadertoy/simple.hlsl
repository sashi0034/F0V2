#define PI 3.14159265359
#define EPS 1e-4
#define MAX_DIST 1000.0
#define MAX_RAYMARCH 64

// 出力
RWTexture2D<float4> g_output : register(u0);

// 入力
// Texture2D<float4> g_texture0 : register(t0);
//
// SamplerState g_sampler0 : register(s0);

cbuffer Shadertoy_b10 : register(b0)
{
    float2 g_screenResolution;
    float2 g_mousePosition;
    float g_time;
};

// -----------------------------------------------

struct SdfAndMat
{
    float sdf;
    float mat;
};

SdfAndMat emptySdfAndMat()
{
    SdfAndMat r;
    r.sdf = 1e10;
    r.mat = 0.0;
    return r;
}

float sdfSphere(float3 p, float r)
{
    return length(p) - r;
}

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

RaycastResult raycast(float3 pos, float3 dir)
{
    RaycastResult r;
    r.pos = 0;
    r.d = emptySdfAndMat();

    float t = 0;
    for (int i = 0; i < MAX_RAYMARCH; ++i)
    {
        float3 p = pos + dir * t;
        SdfAndMat d = scanSdf(p);
        if (d.sdf < EPS)
        {
            r.pos = p;
            r.d = d;
            break;
        }
        t += d.sdf;
        if (t > MAX_DIST) break;
    }

    return r;
}

float4 rayMarch(float3 eyePos, float3 rayDir)
{
    RaycastResult r = raycast(eyePos, rayDir);
    if (r.d.mat > 0)
    {
        return float4(1, 1, 0, 1);
    }

    return float4(0, 0, 1, 1);
}

// -----------------------------------------------

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pixel = dispatchThreadID.xy;
    if (pixel.x >= g_screenResolution.x || pixel.y >= g_screenResolution.y)
    {
        return;
    }

    float2 screenPos2 = (float2(pixel) - g_screenResolution * 0.5) / g_screenResolution.y;
    float3 screenPos3 = float3(screenPos2, 0.0);
    float3 eyePos = float3(0, 0, -5);
    float3 rayDir = normalize(screenPos3 - eyePos);

    // -----------------------------------------------

    g_output[pixel] = rayMarch(eyePos, rayDir);
}
