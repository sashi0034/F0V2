#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

#define FAR_DIST 1e+4f
#define MAX_RAYMARCH 50

#define V2(x) float2((x), (x))
#define V3(x) float3((x), (x), (x))

#define SQ(x) ((x) * (x))

#define repeat(p, span) ((frac(p / span) - 0.5) * span)
#define center_repeat(p, span) (frac((p + span * 0.5) / span) * span - span * 0.5)

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
    float2 g_mouseUV;
    float g_time;
}

// -----------------------------------------------
// math

float hash31(float3 p)
{
    float h = dot(p, float3(127.1, 311.7, 74.7));
    return frac(sin(h) * 43758.5453123);
}

float smoothMin(float a, float b, float k)
{
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return lerp(b, a, h) - k * h * (1.0 - h);
}

// -----------------------------------------------
// matrix

float3x3 rollPitchYaw(float roll, float pitch, float yaw)
{
    float cp = cos(pitch), sp = sin(pitch); // X
    float sr = sin(roll), cr = cos(roll); // Z
    float sy = sin(yaw), cy = cos(yaw); // Y

    return float3x3(
        cr * cy - sr * sp * sy, -sr * cp, cr * sy + sr * sp * cy,
        sr * cy + cr * sp * sy, cr * cp, sr * sy - cr * sp * cy,
        -cp * sy, sp, cp * cy
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

float2x2 rotate2d(float a)
{
    const float s = sin(a), c = cos(a);
    return float2x2(c, -s, s, c);
}

float2 pmod(float2 p, float r)
{
    float a = atan2(p.y, p.x) + PI / r;

    const float n = 2.0 * PI / r;
    a = floor(a / n) * n;

    return mul(rotate2d(-a), p);
}

float2 safe_pmod(float2 p, float r)
{
    float2 result;
    if (p.x == 0 && p.y == 0)
    {
        result = float2(0, 0);
    }
    else
    {
        result = pmod(p, r);
    }

    return result;
}

// -----------------------------------------------
// material

static const float MAT_SOLID = 1.0;

// TODO: Remove this?
struct MatData
{
    float3 albedo;
};

MatData getMatData(float mat)
{
    MatData r;
    if (mat == MAT_SOLID)
    {
        r.albedo = float3(1, 1, 1);
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

float sdfCylinder(float3 p, float h, float r)
{
    float2 d = abs(float2(length(p.xz), p.y)) - float2(r, h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

// https://www.shadertoy.com/view/ltS3W3
float sdfMenger(float3 p)
{
    const int Iterations = 6;
    const float3 offset = float3(1, 1, 1);

    const float time = g_time * 5.0;

    // 初期スケールと動的変化
    p *= 0.5;
    const float scale = 3.5 + 0.5 * sin(time * 0.084);

    // 回転角度を時間で変化させる
    const float rotX = 5.0 * sin(time * 0.01);
    const float rotY = 5.0 * sin(time * 0.0057);
    const float rotZ = 5.0 * sin(time * 0.0266);
    const float3x3 rot = rollPitchYaw(rotX, rotY, rotZ);

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

static float g_mandelbulb_t0;

// https://www.shadertoy.com/view/MdXSWn
float sdfP(float3 p)
{
    p.zx = safe_pmod(p.zx, 5.0);

    p.z -= 2.0;

    p.xyz = p.xzy;
    float3 z = p;
    float3 dz = V3(0.0);
    float power = 8.0;
    float r, theta, phi;
    float dr = 1.0;

    float t0 = 1.0;
    for (int i = 0; i < 7; ++i)
    {
        r = length(z);
        if (r > 2.0)
        {
            continue;
        }

        theta = atan(z.y / z.x);
        phi = asin(z.z / r);
        dr = pow(r, power - 1.0) * dr * power + 1.0;

        r = pow(r, power);
        theta = theta * power;
        phi = phi * power;

        z = r * float3(cos(theta) * cos(phi), sin(theta) * cos(phi), sin(phi)) + p;
        z.z -= sin(5.0 * g_time);

        t0 = min(t0, r);
    }

    g_mandelbulb_t0 = t0;

    return 0.5 * log(r) * r / dr;
}

float sdfO(float3 p)
{
    return sdfP(p);
    return max(sdfP(p), -sdfSphere(p, 5.0));

    // return max(DE(p), -sdfSphere(p, 5.0));
    // return smoothMin(DE(p, 5), DE(p, 10), 0.5 + 0.5 * sin(g_time * 3.0));
}

float sdfTree(float3 p)
{
    float scale = 0.8;
    float3 size = float3(0.1, 1.0, 0.1);
    float d = sdfBox(p, size);
    for (int i = 0; i < 7; i++)
    {
        float3 q = abs(p);
        q.y -= size.y;
        q.xy = mul(rotate2d(0.5), q.xy);
        d = min(d, sdfBox(p, size));
        p = q;
        size *= scale;
    }

    return d;
}

SdfAndMat scanSdf(float3 pos)
{
    SdfAndMat result = emptySdfAndMat();

    // float dSphere = sdfSphere(pos - float3(0, 0, 0), 0.1);
    // if (dSphere < result.sdf)
    // {
    //     result.sdf = dSphere;
    //     result.mat = 1.0;
    // }

    float sdO;
    {
        sdO = sdfO(pos);
    }

    if (sdO < result.sdf)
    {
        result.sdf = sdO;
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

float3 phongLight(float3 eyePos, float3 rayDir, float3 hitPos)
{
    float t0 = g_mandelbulb_t0;

    const float3 N = scanNormal(hitPos);

    const float3 viewDir = normalize(eyePos - hitPos);

    // Light direction
    float3 L = normalize(float3(-0.5, 1.0, -0.5));

    // Diffuse term
    float NoL = saturate(dot(N, L));

    // Specular term (Phong)
    float3 reflectDir = reflect(-L, N);
    float spec = pow(saturate(dot(viewDir, reflectDir)), 32.0);

    // Combine

    float3 c = lerp(float3(0.65, 0.37, 0.63), float3(0.94, 0.6, 0.24), 0.5 + 0.5 * sin(2.0 * t0));

    float3 diffuse = c * NoL + float3(0.18, 0.04, 0.24) * (1 - NoL);
    float3 specular = spec * float3(1.0, 0.9, 0.8);

    float3 color = (diffuse + specular);

    // Soft shadow
    // float shadow = softShadow(pos + N * 1e-2, L, 0.1, 20.0);
    // color *= shadow;

    // Ambient term
    color += V3(0.1);

    return color;
}

float4 rayMarch(float3 eyePos, float3 rayDir)
{
    float3 color = float3(0, 0, 0);
    RaycastResult r = raycast(eyePos, rayDir);

    if (r.d.mat == MAT_SOLID)
    {
        color = phongLight(eyePos, rayDir, r.pos);
    }
    else
    {
        // 背景色
        float tbg = 0.5 * (rayDir.y + 1.0);
        color = lerp(float3(0.93, 0.55, 0.26), float3(0.67, 0.78, 0.91), tbg);
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

    const float roll = sin(g_time) * (HALF_PI * 0.125f);
    const float pitch = sin(g_time * 0.9 + 0.1) * (HALF_PI * 0.125f);
    const float3x3 cameraMat = rollPitchYaw(roll, pitch, sin(g_time));

    float2 screenPos2 = (pixelF - g_screenResolution * 0.5) / g_screenResolution.y;
    screenPos2 *= float2(1.0, -1.0);
    screenPos2 *= 1.5f;

    float3 screenPos3 = float3(screenPos2, 1.0);
    screenPos3 = mul(cameraMat, screenPos3);

    const float3 eyePos = float3(0, 0, 0);

    const float3 rayDir = normalize(screenPos3 - eyePos);

    // -----------------------------------------------

    g_output[pixel] = rayMarch(eyePos, rayDir);
}
