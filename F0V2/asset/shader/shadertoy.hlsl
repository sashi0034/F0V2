Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

// cbuffer SceneState : register(b0)
// {
//     float4x4 g_projectionMatrix;
//     float4x4 g_viewMatrix;
// }

cbuffer Shadertoy_b10 : register(b10)
{
    float2 g_screenResolution;
    float2 g_mousePosition;
    float2 g_jitterOffset;
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

    float dSphere = sdfSphere(pos - float3(0, 0, 0), 0.1);
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

// -----------------------------------------------

float4 PS(PSInput input) : SV_TARGET
{
    float2 inputPosition = input.position.xy + g_jitterOffset;

    float2 screenPos2 = inputPosition.xy;
    screenPos2 = (screenPos2 - g_screenResolution * 0.5) / g_screenResolution.y;

    float3 screenPos3 = float3(screenPos2.x, screenPos2.y, 0.0);
    float3 eyePos = float3(0, 0, -5);

    // screenPos3 = mul(cameraMat, screenPos3);
    // eyePos = mul(cameraMat, eyePos);

    float3 rayDir = normalize(screenPos3 - eyePos);

    RaycastResult r = scanRaycast(eyePos, rayDir);

    float3 color = float3(0, 0, 0);
    if (r.d.mat > 0)
    {
        float3 normal = scanNormal(r.pos);
        color = normal * 0.5 + 0.5;
    }

    // if ((input.position.x + input.position.y) % 2 == 0)
    // {
    //     color.rgb = float3(1, 0.1, 0);
    // }
    // else
    // {
    //     color.rgb = float3(0, 0.1, 1);
    // }

    return float4(color, 1.0);
}
