RWTexture2D<float4> g_velocityField : register(u0);

cbuffer VelocityFieldParams : register(b0)
{
    float2 g_textureResolution;
    float g_elapsedTime;
    float g_terrainSize;
}

static const float2 Gradients[8] =
{
    float2(1.0f, 0.0f),
    float2(-1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(0.0f, -1.0f),
    float2(0.70710678f, 0.70710678f),
    float2(-0.70710678f, 0.70710678f),
    float2(0.70710678f, -0.70710678f),
    float2(-0.70710678f, -0.70710678f),
};

uint HashLattice(int2 lattice)
{
    uint hash = asuint(lattice.x) * 0x8da6b343u;
    hash ^= asuint(lattice.y) * 0xd8163841u;
    hash ^= hash >> 13;
    hash *= 0x85ebca6bu;
    return hash ^ (hash >> 16);
}

float GradientDot(int2 lattice, float2 position)
{
    const float2 gradient = Gradients[HashLattice(lattice) & 7u];
    return dot(gradient, position);
}

float CornerContribution(int2 lattice, float2 position)
{
    float contribution = 0.5f - dot(position, position);
    if (contribution <= 0.0f)
    {
        return 0.0f;
    }

    contribution *= contribution;
    return contribution * contribution * GradientDot(lattice, position);
}

// float SimplexNoise2D(float2 position)
// {
//     static const float F2 = 0.3660254037844386f;
//     static const float G2 = 0.2113248654051871f;
//
//     const float skew = (position.x + position.y) * F2;
//     const int2 lattice = int2(floor(position + skew));
//     const float unskew = float(lattice.x + lattice.y) * G2;
//     const float2 position0 = position - (float2(lattice) - unskew);
//
//     const int2 latticeOffset = position0.x > position0.y ? int2(1, 0) : int2(0, 1);
//     const float2 position1 = position0 - float2(latticeOffset) + G2;
//     const float2 position2 = position0 - 1.0f + 2.0f * G2;
//
//     return 70.0f * (
//         CornerContribution(lattice, position0)
//         + CornerContribution(lattice + latticeOffset, position1)
//         + CornerContribution(lattice + 1, position2));
// }

static const float3 Gradients3D[12] =
{
    float3(1.0f, 1.0f, 0.0f),
    float3(-1.0f, 1.0f, 0.0f),
    float3(1.0f, -1.0f, 0.0f),
    float3(-1.0f, -1.0f, 0.0f),
    float3(1.0f, 0.0f, 1.0f),
    float3(-1.0f, 0.0f, 1.0f),
    float3(1.0f, 0.0f, -1.0f),
    float3(-1.0f, 0.0f, -1.0f),
    float3(0.0f, 1.0f, 1.0f),
    float3(0.0f, -1.0f, 1.0f),
    float3(0.0f, 1.0f, -1.0f),
    float3(0.0f, -1.0f, -1.0f),
};

uint HashLattice(int3 lattice)
{
    uint hash = asuint(lattice.x) * 0x8da6b343u;
    hash ^= asuint(lattice.y) * 0xd8163841u;
    hash ^= asuint(lattice.z) * 0xcb1ab31fu;
    hash ^= hash >> 13;
    hash *= 0x85ebca6bu;
    return hash ^ (hash >> 16);
}

float GradientDot(int3 lattice, float3 position)
{
    const float3 gradient = Gradients3D[HashLattice(lattice) % 12u];
    return dot(gradient, position);
}

float CornerContribution(int3 lattice, float3 position)
{
    float contribution = 0.6f - dot(position, position);
    if (contribution <= 0.0f)
    {
        return 0.0f;
    }

    contribution *= contribution;
    return contribution * contribution * GradientDot(lattice, position);
}

float SimplexNoise3D(float3 position)
{
    static const float F3 = 1.0f / 3.0f;
    static const float G3 = 1.0f / 6.0f;

    const float skew = (position.x + position.y + position.z) * F3;
    const int3 lattice = int3(floor(position + skew));
    const float unskew = float(lattice.x + lattice.y + lattice.z) * G3;
    const float3 position0 = position - (float3(lattice) - unskew);

    int3 latticeOffset1;
    int3 latticeOffset2;
    if (position0.x >= position0.y)
    {
        if (position0.y >= position0.z)
        {
            latticeOffset1 = int3(1, 0, 0);
            latticeOffset2 = int3(1, 1, 0);
        }
        else if (position0.x >= position0.z)
        {
            latticeOffset1 = int3(1, 0, 0);
            latticeOffset2 = int3(1, 0, 1);
        }
        else
        {
            latticeOffset1 = int3(0, 0, 1);
            latticeOffset2 = int3(1, 0, 1);
        }
    }
    else
    {
        if (position0.y < position0.z)
        {
            latticeOffset1 = int3(0, 0, 1);
            latticeOffset2 = int3(0, 1, 1);
        }
        else if (position0.x < position0.z)
        {
            latticeOffset1 = int3(0, 1, 0);
            latticeOffset2 = int3(0, 1, 1);
        }
        else
        {
            latticeOffset1 = int3(0, 1, 0);
            latticeOffset2 = int3(1, 1, 0);
        }
    }

    const float3 position1 = position0 - float3(latticeOffset1) + G3;
    const float3 position2 = position0 - float3(latticeOffset2) + 2.0f * G3;
    const float3 position3 = position0 - 1.0f + 3.0f * G3;

    return 32.0f * (
        CornerContribution(lattice, position0)
        + CornerContribution(lattice + latticeOffset1, position1)
        + CornerContribution(lattice + latticeOffset2, position2)
        + CornerContribution(lattice + 1, position3));
}

float NormalizedFBm(float2 worldPosition, float time)
{
    static const float BaseFrequency = 0.025f;
    static const float2 NoiseOffset = float2(73.0f, -41.0f);

    float amplitude = 1.0f;
    float frequency = BaseFrequency;
    float value = 0.0f;
    float amplitudeSum = 0.0f;

    for (int octave = 0; octave < 4; ++octave)
    {
        float3 p = float3(
            (worldPosition + NoiseOffset) * frequency,
            time * 0.05f * (frequency / BaseFrequency)
        );

        value += SimplexNoise3D(p) * amplitude;
        amplitudeSum += amplitude;

        frequency *= 2.0f;
        amplitude *= 0.5f;
    }

    return value / amplitudeSum;
}

float2 CurlNoise(float2 worldPosition, float time)
{
    static const float DerivativeStep = 0.25f;
    static const float VelocityScale = 25.0f;

    const float dNoiseDx = (
        NormalizedFBm(worldPosition + float2(DerivativeStep, 0), time)
        - NormalizedFBm(worldPosition - float2(DerivativeStep, 0), time)
    ) / (DerivativeStep * 2.0f);

    const float dNoiseDz = (
        NormalizedFBm(worldPosition + float2(0, DerivativeStep), time)
        - NormalizedFBm(worldPosition - float2(0, DerivativeStep), time)
    ) / (DerivativeStep * 2.0f);

    return float2(dNoiseDz, -dNoiseDx) * VelocityScale;
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixel = dispatchThreadID.xy;
    const uint2 resolution = uint2(g_textureResolution);
    if (pixel.x >= resolution.x || pixel.y >= resolution.y)
    {
        return;
    }

    const float2 uv = (float2(pixel) + 0.5f) / g_textureResolution;
    float2 worldPosition = (uv - 0.5f) * g_terrainSize;

    const float2 velocity = CurlNoise(worldPosition, g_elapsedTime);
    g_velocityField[pixel] = float4(velocity, 0.0f, 1.0f);
}
