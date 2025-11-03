#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

#define FAR_DIST 1000.0
#define MAX_RAYMARCH 50

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
}

// -----------------------------------------------
// random

float hash31(float3 p)
{
    float h = dot(p, float3(127.1, 311.7, 74.7));
    return frac(sin(h) * 43758.5453123);
}

// -----------------------------------------------
// matrix

// Roll-Pitch-Yaw行列を作成
float3x3 rotateRollPitchYaw(float roll, float pitch, float yaw)
{
    float cp = cos(pitch);
    float sp = sin(pitch);
    float sr = sin(roll);
    float cr = cos(roll);
    float sy = sin(yaw);
    float cy = cos(yaw);

    return float3x3(
        cp * cy, (sr * sp * cy) - (cr * sy), (cr * sp * cy) + (sr * sy),
        cp * sy, (sr * sp * sy) + (cr * cy), (cr * sp * sy) - (sr * cy),
        -sp, sr * cp, cr * cp
    );
}

float3x3 rotateAxis(float3 axis, float angle)
{
    float s = sin(angle);
    float c = cos(angle);
    float t = 1.0 - c;

    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    return float3x3(
        t * x * x + c, t * x * y - s * z, t * x * z + s * y,
        t * x * y + s * z, t * y * y + c, t * y * z - s * x,
        t * x * z - s * y, t * y * z + s * x, t * z * z + c
    );
}

float3x3 rotateY(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return float3x3(
        c, 0, s,
        0, 1, 0,
        -s, 0, c
    );
}

// -----------------------------------------------
// material

static const float MAT_SOLID = 1.0;

struct MatData
{
    float3 albedo;
};

MatData getMatData(float mat)
{
    MatData r;
    if (mat == MAT_SOLID)
    {
        r.albedo = float3(0.04, 1, 0.86);
    }
    else
    {
        r.albedo = float3(0, 0, 0);
    }

    return r;
}

// -----------------------------------------------
// sdf

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

float sdfBox(float3 p, float3 b)
{
    float3 d = abs(p) - b;
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float2x2 rotate(float a)
{
    float s = sin(a), c = cos(a);
    return float2x2(c, s, -s, c);
}

// https://www.shadertoy.com/view/ltS3W3
float sdfMenger(float3 p, float3 idx)
{
    const int Iterations = 6;
    float3 offset = float3(1, 1, 1);

    // float time = g_time + idx.x * 10.0 + idx.y * 20.0 + idx.z * 30.0;
    const float time = (1.0 + g_time) * (1.0 + hash31(idx)) * 5.0;

    // 初期スケールと動的変化
    p *= 0.4;
    float scale = 3.5 + 0.3 * sin(time * 0.084);

    // 回転角度を時間で変化させる
    float rotX = 5.0 * sin(time * 0.01);
    float rotY = 5.0 * sin(time * 0.0057);
    float rotZ = 5.0 * sin(time * 0.0266);
    float3x3 rot = rotateRollPitchYaw(rotX, rotY, rotZ);

    // 反復折り返し
    [unroll]
    for (int i = 0; i < Iterations; i++)
    {
        p = mul(p, rot);

        // 軸対称化
        p = abs(p);

        // fold
        if (p.x - p.y < 0.0) p.xy = p.yx;
        if (p.x - p.z < 0.0) p.xz = p.zx;
        if (p.y - p.z < 0.0) p.yz = p.zy;

        // Z方向の折り返し
        p.z -= 0.5 * offset.z * (scale - 1.0) / scale;
        p.z = -abs(-p.z);
        p.z += 0.5 * offset.z * (scale - 1.0) / scale;

        // スケーリングと平行移動
        p.xy = scale * p.xy - offset.xy * (scale - 1.0); // (p - offset) * scale + offset; // offset 中心拡大
        p.z = scale * p.z;
    }

    // 距離評価
    float3 d = abs(p) - 1.0;
    float distance = min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));
    distance *= pow(scale, -Iterations);

    return distance;
}

// float sdfTree(float3 p)
// {
//     float scale = 0.8;
//     float3 size = float3(0.1, 1.0, 0.1);
//     float d = sdfBox(p, size);
//     for (int i = 0; i < 7; i++)
//     {
//         float3 q = abs(p);
//         q.y -= size.y;
//         q.xy = mul(rotate(-0.5), q.xy);
//         d = min(d, sdfBox(p, size));
//         p = q;
//         size *= scale;
//     }
//
//     return d;
// }

SdfAndMat scanSdf(float3 pos)
{
    SdfAndMat result = emptySdfAndMat();

    // float dSphere = sdfSphere(pos - float3(0, 0, 0), 0.1);
    // if (dSphere < result.sdf)
    // {
    //     result.sdf = dSphere;
    //     result.mat = 1.0;
    // }

    float sdMenger;
    {
        float3 cell = float3(1, 1, 1) * 8.0;
        float3 folded = frac(pos / cell) * cell - 0.5 * cell;
        float3 idx = floor(pos / cell);
        float3 p = folded - float3(0, 0, 5.0f);
        sdMenger = sdfMenger(p, idx);
    }

    if (sdMenger < result.sdf)
    {
        result.sdf = sdMenger;
        result.mat = 1.0;
    }

    return result;
}

float3 scanNormal(float3 pos)
{
    const float h = 1e-2;
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
        if (d.sdf < 1e-2)
        {
            r.pos = p;
            r.d = d;
            break;
        }

        t += d.sdf;
        if (t > FAR_DIST) break;
    }

    return r;
}

// -----------------------------------------------
// lighting

float softShadow(float3 ro, float3 rd, float mint, float k)
{
    float result = 1.0;
    float t = mint;

    [loop]
    for (int i = 0; i < 48; i++)
    {
        float h = scanSdf(ro + rd * t).sdf;
        h = max(h, 0.0);
        result = min(result, k * h / t);
        t += clamp(h, 0.01, 0.5);
        if (t > FAR_DIST)
        {
            break;
        }
    }

    return clamp(result, 0.0, 1.0);
}

// Lambert + Phong specular
float3 applyLight(float3 pos, float3 N, float3 viewDir, float3 albedo)
{
    // Light direction
    float3 L = normalize(float3(-0.5, 1.0, -0.5));

    // Diffuse term
    float NoL = saturate(dot(N, L));

    // Soft shadow
    float shadow = softShadow(pos + N * 1e-2, L, 0.1, 20.0);

    // Specular term (Phong)
    float3 reflectDir = reflect(-L, N);
    float spec = pow(saturate(dot(viewDir, reflectDir)), 32.0);

    // Combine
    float3 diffuse = albedo * NoL;
    float3 specular = spec * float3(1.0, 0.9, 0.8);

    float3 color = (diffuse + specular) * shadow;

    // Ambient term
    color += albedo * 0.1;

    return color;
}

float4 rayMarch(float3 eyePos, float3 rayDir)
{
    float3 color = float3(0, 0, 0);
    RaycastResult r = raycast(eyePos, rayDir);

    if (r.d.mat > 0)
    {
        float3 n = scanNormal(r.pos);

        MatData mat = getMatData(r.d.mat);

        float3 viewDir = normalize(eyePos - r.pos);
        color = applyLight(r.pos, n, viewDir, mat.albedo);
    }
    else
    {
        // 背景色
        float tbg = 0.5 * (rayDir.y + 1.0);
        color = lerp(float3(0.2, 0.4, 0.5), float3(1.0, 0.9, 0.7), tbg);
    }

    // float3 fogColor = float3(0.5, 0.7, 0.9);
    // float fogDensity = 0.05;
    //
    // const float t = distance(eyePos, r.pos);
    // float fogFactor = 1.0 - exp(-t * fogDensity);
    // color = lerp(color, fogColor, fogFactor);

    return float4(color, 1);
}

// -----------------------------------------------

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixel = dispatchThreadID.xy;
    const float2 pixelF = float2(pixel);
    if (pixelF.x >= g_screenResolution.x || pixelF.y >= g_screenResolution.y)
    {
        return;
    }

    const float3x3 cameraMat = rotateY(sin(g_time) * (HALF_PI * 0.25f));

    float2 screenPos2 = (pixelF - g_screenResolution * 0.5) / g_screenResolution.y;
    screenPos2 *= 1.5f;

    float3 screenPos3 = float3(screenPos2, 1.0);
    screenPos3 = mul(cameraMat, screenPos3);

    const float3 eyePos = float3(0, 0, 0);

    const float3 rayDir = normalize(screenPos3 - eyePos);

    // -----------------------------------------------

    g_output[pixel] = rayMarch(eyePos, rayDir);
}
