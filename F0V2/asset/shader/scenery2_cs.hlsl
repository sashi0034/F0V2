#define PI 3.14159265359
#define TWO_PI (PI * 2)
#define HALF_PI (PI * 0.5)

#define V2(x) float2((x), (x))
#define V3(x) float3((x), (x), (x))

#define SQ(x) ((x) * (x))

#define repeat(p, span) ((frac((p) / (span)) - 0.5) * (span))
#define center_repeat(p, span) (frac(((p) + (span) * 0.5) / (span)) * (span) - (span) * 0.5)

// -----------------------------------------------

#define MAX_RAYMARCH 128

// -----------------------------------------------

SamplerState g_sampler0 : register(s0);

SamplerComparisonState g_shadowMapSampler : register(s1);

Texture2D<float4> g_albedoBuffer : register(t0);

Texture2D<float4> g_normalBuffer : register(t1);

Texture2D<float> g_viewDistanceBuffer : register(t2);

Texture2D<float> g_depthBuffer : register(t3);

Texture2D<float4> g_shadowMap : register(t4);

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

// from sRGB to Linear (approximate)
float3 sRGB2L_(float3 srgb)
{
    return srgb * (srgb * (srgb * 0.305306011 + 0.682171111) + 0.012522878);
}

// from Linear to sRGB
float3 L2sRGB(float3 rgb)
{
    float3 lt = step(float3(0.0031308, 0.0031308, 0.0031308), rgb);
    float3 low = rgb * 12.92;
    float3 high = 1.055 * pow(rgb, 1.0 / 2.4) - 0.055;
    return lerp(low, high, lt);
}

// from Linear to sRGB (approximate)
float3 L2sRGB_(float3 linear_)
{
    float3 x1 = sqrt(linear_);
    float3 x2 = sqrt(x1);
    float3 x3 = sqrt(x2);
    return 0.662002687 * x1 + 0.684122060 * x2 - 0.323583601 * x3 - 0.0225411470 * linear_;
}

// -----------------------------------------------
// material

typedef float MatId;

static const MatId MAT_SOLID = 1.0;
static const MatId MAT_VEHICLE = 2.0;

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
    MatId mat;
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

float sdSquare(float2 p, float r)
{
    float2 d = abs(p) - V2(r);
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
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

// 地面
float sdfV(float3 p)
{
    float base = p.y + 200.0f;

    float n = sin(p.x * 0.05 + g_time) * cos(p.z * (0.025 + 0.0125 * sin(g_time * 0.5)));
    base += n * 20.0;

    return base;
}

// https://www.shadertoy.com/view/MdXSWn
float sdfO(float3 p)
{
    // p.zx = frac((p.zx + V2(20.0)) / 40.0) * 40.0 - V2(20.0);

    const float Scale0 = 0.05;
    p *= Scale0;

    p.zx = center_repeat(p.zx, 37.5);

    p.zx = pmod(p.zx, 5.0);
    p.zy = pmod(p.zy, 5.0);

    // p = abs(p);
    //
    // if (p.z < p.y) p.zy = p.yz;
    // if (p.z < p.x) p.zx = p.xz;
    // if (p.y < p.x) p.yx = p.xy;

    p -= float3(0, 0, 10);
    // p.zy = mul(rotate2d(g_time), p.zy);

    float h = 2.0;
    float r = 0.1;
    float d = sdfCylinder(p, h, r);
    const float Scale = 0.8;
    const int Iterations = 20;

    for (int i = 0; i < Iterations; i++)
    {
        if (i == Iterations - 1)
        {
            float d_ = smoothMin(d, sdfCylinder(p, h, r), 0.1);
            d = lerp(d, d_, 0.5);
            break;
        }
        else
        {
            d = smoothMin(d, sdfCylinder(p, h, r), 0.1);
        }

        p = abs(p);
        // p.xz = abs(p.xz);
        p.y -= h;
        p.zx = mul(rotate2d(HALF_PI * (0.2 + 0.1 * sin(g_time * 3.0))), p.zx);
        p.xy = mul(rotate2d(0.45), p.xy);

        h *= Scale;
        r *= Scale;
    }

    return d / Scale0;
}

SdfAndMat scanSdf(float3 pos)
{
    SdfAndMat result = emptySdfAndMat();

    float sdO = sdfO(pos);
    if (sdO < result.sdf)
    {
        result.sdf = sdO;
        result.mat = MAT_SOLID;
    }

    float sdV = sdfV(pos);
    if (sdV < result.sdf)
    {
        result.sdf = sdV;
        result.mat = MAT_VEHICLE;
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

enum RaycastTag
{
    RAYCAST_MISS,
    RAYCAST_HIT_SDF,
    RAYCAST_HIT_LIMIT
};

struct RaycastResult
{
    RaycastTag tag;
    float3 pos;
    float distance;
    SdfAndMat d;
};

RaycastResult raycast(float3 pos, float3 dir, float distanceLimit, bool hasHint, float initialDistanceHint)
{
    RaycastResult r;
    r.tag = RAYCAST_MISS;
    r.pos = 0;
    r.distance = 0;
    r.d = emptySdfAndMat();

    const int maxSteps = hasHint ? 2 : MAX_RAYMARCH;
    const float eps = 0.1; // hasHint ? 0.1 : 1e-2f;

    float t = initialDistanceHint;
    for (int i = 0; i < maxSteps; ++i)
    {
        float3 p = pos + dir * t;
        SdfAndMat d = scanSdf(p);
        if (d.sdf < eps)
        {
            r.tag = RAYCAST_HIT_SDF;
            r.pos = p;
            r.distance = t;
            r.d = d;
            break;
        }

        t += d.sdf;
        if (t > distanceLimit)
        {
            r.tag = RAYCAST_HIT_LIMIT;
            r.distance = distanceLimit;
            break;
        }

        r.distance = t;
    }

    return r;
}

// -----------------------------------------------
// lighting

float3 computeSkyColor(float3 V)
{
    const float t = 0.5 * (-V.y + 1.0);
    return lerp(sRGB2L(0.93, 0.55, 0.26), sRGB2L(0.67, 0.78, 0.91), t);
}

struct LightParameters
{
    float3 dir;
    float3 color;
};

static const int NUM_LIGHTS = 4;

static const LightParameters k_lightList[NUM_LIGHTS] = {
    {normalize(float3(-0.4, 1.0, -0.3)), sRGB2L(1.0, 0.95, 0.85) * 0.5},
    {normalize(float3(0.5, 1.0, 0.4)), sRGB2L(0.8, 0.9, 1.0) * 0.4},
    {normalize(float3(-0.3, -0.6, 0.6)), sRGB2L(0.9, 1.0, 0.9) * 0.3},
    {normalize(float3(0.4, -0.5, -0.5)), sRGB2L(1.0, 0.8, 0.9) * 0.3},
};

struct LightingInput
{
    float3 worldPos;
    float3 N;
    float3 V;
    float3 albedo;
    float viewDistance;
};

float3 computeLighting(LightingInput input)
{
    float3 light = V3(0.0f);

    for (int i = 0; i < NUM_LIGHTS; ++i)
    {
        // Light direction
        const float3 L = k_lightList[i].dir;

        // Diffuse term
        const float NoL = saturate(dot(input.N, L));
        const float3 diffuseTerm = k_lightList[i].color * NoL;

        // Specular term (Phong)
        const float3 R = reflect(-L, input.N);
        const float specularFactor = pow(saturate(dot(input.V, R)), 32.0);
        const float3 specularTerm = k_lightList[i].color * specularFactor;

        light += diffuseTerm + specularTerm;
    }

    // Shadow
    const float4 shadowP = mul(g_worldToShadowProjection, float4(input.worldPos, 1));
    const float2 shadowUV = (shadowP.xy / shadowP.w) * float2(0.5f, -0.5f) + float2(0.5f, 0.5f); // [-1, 1] -> [0, 1]
    if (all(0.0 <= shadowUV) && all(shadowUV <= 1.0))
    {
        const float currentDepth = shadowP.z / shadowP.w;

        // const float shadowDepth = g_shadowMap.SampleLevel(g_sampler0, shadowUV, 0.0).r; // FIXME: Use SampleCmpLevelZero
        // if (shadowDepth < currentDepth - 1e-3f)
        // {
        //     light *= 0.5;
        // }

        const float shadowValue = g_shadowMap.SampleCmpLevelZero(g_shadowMapSampler, shadowUV, currentDepth - 1e-3f);
        light *= lerp(1.0f, 0.5f, shadowValue);
    }

    // Combine
    const float3 ambientTerm = float3(0.10, 0.05, 0.10);
    float3 color = input.albedo * (light + ambientTerm);

    // Fog
    float fogStart = 10.0;
    float fogEnd = 500.0;

    float fogFactor = saturate((input.viewDistance - fogStart) / (fogEnd - fogStart));

    color = lerp(color, computeSkyColor(input.V), fogFactor);

    return color;
}

enum RayMarchTag
{
    RAY_MARCH_MISS,
    RAY_MARCH_HIT_GBUFFER,
    RAY_MARCH_HIT_SDF,
    RAY_MARCH_SKY,
};

struct RayMarchResult
{
    int tag;
    float distance;
    LightingInput lighting;
};

RayMarchResult rayMarch(
    float3 eyePos,
    float3 rayDir,
    float distanceLimit,
    bool pixelAlreadyExists,
    bool hasHint,
    float initialDistanceHint)
{
    RayMarchResult result;

    const RaycastResult r = raycast(eyePos, rayDir, distanceLimit, hasHint, initialDistanceHint);

    result.distance = r.distance;

    if (r.tag == RAYCAST_MISS)
    {
        result.tag = RAY_MARCH_MISS;
        return result;
    }

    if (r.tag == RAYCAST_HIT_LIMIT && pixelAlreadyExists)
    {
        result.tag = RAY_MARCH_HIT_GBUFFER;
        return result;
    }

    const float3 V = -rayDir;

    if (r.d.mat == MAT_SOLID ||
        r.d.mat == MAT_VEHICLE)
    {
        float3 albedo;
        if (r.d.mat == MAT_SOLID)
        {
            albedo = sRGB2L(0.83, 0.7, 0.24);
        }
        else // if (r.d.mat == MAT_VEHICLE)
        {
            albedo = sRGB2L(1, 0.47, 0.03);
        }

        const float3 N = scanNormal(r.pos);

        result.tag = RAY_MARCH_HIT_SDF;
        result.lighting.worldPos = r.pos;
        result.lighting.N = N;
        result.lighting.V = V;
        result.lighting.albedo = albedo;
        result.lighting.viewDistance = length(r.pos - eyePos);
        return result;
    }
    else
    {
        result.tag = RAY_MARCH_SKY;
    }

    return result;
}

// -----------------------------------------------

static const int INITIAL_DISTANCE_HINT_CAPACITY = 4;

struct InitialDistanceHint
{
    int count;
    float values[INITIAL_DISTANCE_HINT_CAPACITY];
    float3 fallback;
};

float4 computeOutputColor(uint2 pixel, bool hasHint, InitialDistanceHint initialDistanceHint)
{
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

    const bool pixelAlreadyExists = g_albedoBuffer[pixel].a != 0.0; // フォワードレンダリング時点で値が書き込まれているか
    const float distanceLimit = g_viewDistanceBuffer[pixel]; // 最大で far 値

    RayMarchResult hit;
    if (hasHint)
    {
        bool initialized = false;
        [unroll]
        for (int i = 0; i < INITIAL_DISTANCE_HINT_CAPACITY; i++)
        {
            RayMarchResult hit_ = rayMarch(
                eyePosInWorld, rayDir, distanceLimit, pixelAlreadyExists, hasHint, initialDistanceHint.values[i]);
            if (hit_.tag != RAY_MARCH_MISS && (!initialized || hit_.distance < hit.distance))
            {
                initialized = true;
                hit = hit_;
            }

            if (!initialized && i == initialDistanceHint.count - 1)
            {
                // return float4(1, 0, 0, 1); // debug
                return float4(initialDistanceHint.fallback, 1.0);
            }
        }
    }
    else
    {
        hit = rayMarch(
            eyePosInWorld, rayDir, distanceLimit, pixelAlreadyExists, hasHint, 0);
    }

    float3 rgb;
    if (hit.tag == RAY_MARCH_SKY || hit.tag == RAY_MARCH_MISS)
    {
        rgb = computeSkyColor(-rayDir);
    }
    else // if (hit.tag == RAY_MARCH_GBUFFER || hit.tag == RAY_MARCH_SDF)
    {
        LightingInput lightingInput = hit.lighting;
        if (hit.tag == RAY_MARCH_HIT_GBUFFER)
        {
            // SDF よりもGBuffer が手前にある場合、レイマーチングの SDF を用いずに GBuffer の情報を使う
            lightingInput.worldPos = targetInWorld;
            lightingInput.N = g_normalBuffer[pixel].rgb * 2.0 - 1.0;
            lightingInput.V = -rayDir;
            lightingInput.albedo = g_albedoBuffer[pixel].rgb;
            lightingInput.viewDistance = distanceLimit;
        }

        rgb = computeLighting(lightingInput);
    }

    return float4(L2sRGB_(rgb), hit.distance);
}

static const uint THREADS_PER_TILE = 32;

groupshared float4 gs_firstPathOutput[THREADS_PER_TILE];

[numthreads(32, 1, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint PIXELS_PER_TILE_X = 8;
    const uint PIXELS_PER_TILE_Y = 8;
    const uint THREADS_PER_TILE_X = PIXELS_PER_TILE_X / 2;
    // assert(THREADS_PER_TILE == THREADS_PER_TILE_X * PIXELS_PER_TILE_Y);

    const uint TILE_COLUMNS = g_outputResolution.x / PIXELS_PER_TILE_X;
    const uint tileId = dispatchThreadID.x / THREADS_PER_TILE;
    const uint tileCoordX = tileId % TILE_COLUMNS;
    const uint tileCoordY = tileId / TILE_COLUMNS;

    const uint localThreadId = dispatchThreadID.x % THREADS_PER_TILE;
    const uint offsetInTileX = localThreadId % THREADS_PER_TILE_X;
    const uint offsetInTileY = localThreadId / THREADS_PER_TILE_X;
    const uint isOddY = offsetInTileY % 2;

    uint globalY = tileCoordY * PIXELS_PER_TILE_Y + offsetInTileY;
    uint globalX = tileCoordX * PIXELS_PER_TILE_X + offsetInTileX * 2 + isOddY;

    const uint2 firstCoord = uint2(globalX, globalY);
    if (float(firstCoord.x) >= g_outputResolution.x || float(firstCoord.y) >= g_outputResolution.y)
    {
        return;
    }

#if 0
    // シャドウマップのデバッグ
    const float debugMapSize = 800.0;
    if (all((float2(firstCoord)) < debugMapSize))
    {
        float2 uv = (float2(firstCoord) + 0.5) / debugMapSize;
        float shadow = g_shadowMap.SampleLevel(g_sampler0, uv, 0.0).r;
        if (shadow != 1.0)
        {
            g_output[firstCoord] = float4(1.0, 0.0, 0.0, 1.0);
        }
        else
        {
            g_output[firstCoord] = float4(0.0, 0.0, 0.0, 1.0);
        }

        const int secondOffsetX = isOddY == 0 ? 1 : -1;
        const float2 secondCoord = firstCoord + int2(secondOffsetX, 0);
        g_output[secondCoord] = g_output[firstCoord];

        return;
    }
#endif

    InitialDistanceHint initialDistanceHint;
    initialDistanceHint.count = 0;
    initialDistanceHint.fallback = float3(0, 0, 0);

    const float4 firstOutput = computeOutputColor(firstCoord, false, initialDistanceHint);
    gs_firstPathOutput[localThreadId] = firstOutput;
    g_output[firstCoord] = float4(firstOutput.rgb, 1.0);

    const int secondOffsetX = isOddY == 0 ? 1 : -1;
    const float2 secondCoord = firstCoord + int2(secondOffsetX, 0);

#if 0
    g_output[secondCoord] = float4(computeOutputColor(secondCoord, false, initialDistanceHint).rgb, 1.0);
    return;
#endif

    GroupMemoryBarrierWithGroupSync(); // <-- Barrier 

    initialDistanceHint.values[initialDistanceHint.count] = firstOutput.a;
    initialDistanceHint.fallback = firstOutput.rgb;
    initialDistanceHint.count = 1;

    // left
    if (isOddY && 0 < offsetInTileX)
    {
        initialDistanceHint.values[initialDistanceHint.count] =
            gs_firstPathOutput[localThreadId - 1].a;
        initialDistanceHint.fallback +=
            gs_firstPathOutput[localThreadId - 1].rgb;
        initialDistanceHint.count++;
    }

    // right
    if (!isOddY && offsetInTileX < THREADS_PER_TILE_X - 1)
    {
        initialDistanceHint.values[initialDistanceHint.count] =
            gs_firstPathOutput[localThreadId + 1].a;
        initialDistanceHint.fallback +=
            gs_firstPathOutput[localThreadId + 1].rgb;
        initialDistanceHint.count++;
    }

    // up
    if (0 < offsetInTileY)
    {
        initialDistanceHint.values[initialDistanceHint.count] =
            gs_firstPathOutput[localThreadId - THREADS_PER_TILE_X].a;
        initialDistanceHint.fallback +=
            gs_firstPathOutput[localThreadId - THREADS_PER_TILE_X].rgb;
        initialDistanceHint.count++;
    }

    // down
    if (offsetInTileY < PIXELS_PER_TILE_Y - 1)
    {
        initialDistanceHint.values[initialDistanceHint.count] =
            gs_firstPathOutput[localThreadId + THREADS_PER_TILE_X].a;
        initialDistanceHint.fallback +=
            gs_firstPathOutput[localThreadId + THREADS_PER_TILE_X].rgb;
        initialDistanceHint.count++;
    }

    initialDistanceHint.fallback /= float(initialDistanceHint.count);

    g_output[secondCoord] = float4(computeOutputColor(secondCoord, true, initialDistanceHint).rgb, 1.0);
}
