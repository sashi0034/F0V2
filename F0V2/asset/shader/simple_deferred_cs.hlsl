#define MAX_RAYMARCH 128

SamplerState g_sampler0 : register(s0);

SamplerComparisonState g_shadowMapSampler : register(s1);

Texture2D<float4> g_albedoBuffer : register(t0);

Texture2D<float4> g_normalBuffer : register(t1);

Texture2D<float> g_viewDistanceBuffer : register(t2);

Texture2D<float> g_depthBuffer : register(t3);

// 出力
RWTexture2D<float4> g_output : register(u0);

cbuffer Scenery_b10 : register(b0)
{
    column_major float4x4 g_projectionMatrixInv;
    column_major float4x4 g_viewMatrixInv;
    column_major float4x4 g_worldToShadowProjection;
    float2 g_outputResolution;
    float2 g_mousePosition;
    float2 g_mouseUV;
    float g_time;
}

// -----------------------------------------------

// from sRGB to Linear
float3 sRGB2L(float3 srgb)
{
    float3 cutoff = step(srgb, float3(0.04045, 0.04045, 0.04045));
    float3 higher = pow((srgb + float3(0.055, 0.055, 0.055)) / 1.055, float3(2.4, 2.4, 2.4));
    float3 lower = srgb / 12.92;
    float3 linear_ = lerp(higher, lower, cutoff);
    return linear_;
}

float3 sRGB2L(float r, float g, float b)
{
    return sRGB2L(float3(r, g, b));
}

// from Linear to sRGB (approximate)
float3 L2sRGB_(float3 linear_)
{
    float3 x1 = sqrt(linear_);
    float3 x2 = sqrt(x1);
    float3 x3 = sqrt(x2);
    return 0.662002687 * x1 + 0.684122060 * x2 - 0.323583601 * x3 - 0.0225411470 * linear_;
}

float3 computeSkyColor(float3 V)
{
    const float t = 0.5 * (-V.y + 1.0);
    return lerp(sRGB2L(float3(1.0, 0.7, 1.0)), sRGB2L(float3(0.3, 0.0, 1.0)), t);
}

[numthreads(4, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixel = dispatchThreadID.xy;
    if (pixel.x >= g_outputResolution.x || pixel.y >= g_outputResolution.y)
    {
        return;
    }

    float3 targetInNdc;
    targetInNdc.xy = float2(2.0, -2.0) * float2(pixel) / g_outputResolution + float2(-1.0, 1.0);
    targetInNdc.z = g_depthBuffer[pixel]; // 1.0

    const float4 targetInClip = float4(targetInNdc, 1.0f);

    float4 targetInView = mul(g_projectionMatrixInv, targetInClip);
    targetInView /= targetInView.w;

    // const float distanceLimit = length(targetInView);

    const float3 targetInWorld = mul(g_viewMatrixInv, targetInView).xyz;
    const float3 eyePosInWorld = mul(g_viewMatrixInv, float4(0, 0, 0, 1)).xyz;

    const float3 rayDir = normalize(targetInWorld - eyePosInWorld);

    // -----------------------------------------------

    const float3 N = g_normalBuffer[pixel].rgb * 2.0 - 1.0;
    const float3 V = -rayDir;
    float lambertTerm = max(dot(N, V), 0.0);

    float3 finalColor;
    if (g_albedoBuffer[pixel].a > 0.0)
    {
        finalColor = g_albedoBuffer[pixel].rgb * lambertTerm;
    }
    else
    {
        finalColor = computeSkyColor(V);
    }

    g_output[pixel] = float4(L2sRGB_(finalColor), 1.0);
}
