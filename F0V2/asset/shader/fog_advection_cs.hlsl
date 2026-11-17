Texture2D<float4> g_previousFog : register(t0);
Texture2D<float4> g_velocityField : register(t1);

RWTexture2D<float4> g_nextFog : register(u0);

SamplerState g_linearClamp : register(s0);

cbuffer FogAdvectionParams : register(b0)
{
    float2 g_textureResolution;
    float g_deltaTime;
    float g_terrainSize;
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
    const float4 velocityState = g_velocityField.SampleLevel(g_linearClamp, uv, 0.0f);
    const float2 velocity = velocityState.rg;

    // 速度場の発散を中心差分で求める。
    uint velocityWidth;
    uint velocityHeight;
    g_velocityField.GetDimensions(velocityWidth, velocityHeight);
    const float2 velocityResolution = float2(velocityWidth, velocityHeight);
    const float2 texelSize = 1.0f / velocityResolution;
    const float2 worldTexelSize = g_terrainSize / velocityResolution;
    const float velocityLeft = g_velocityField.SampleLevel(
        g_linearClamp, uv - float2(texelSize.x, 0.0f), 0.0f).r;
    const float velocityRight = g_velocityField.SampleLevel(
        g_linearClamp, uv + float2(texelSize.x, 0.0f), 0.0f).r;
    const float velocityBack = g_velocityField.SampleLevel(
        g_linearClamp, uv - float2(0.0f, texelSize.y), 0.0f).g;
    const float velocityFront = g_velocityField.SampleLevel(
        g_linearClamp, uv + float2(0.0f, texelSize.y), 0.0f).g;
    const float divergence =
        (velocityRight - velocityLeft) / (2.0f * worldTexelSize.x)
        + (velocityFront - velocityBack) / (2.0f * worldTexelSize.y);

    // Semi-Lagrangian 法で前フレームの位置を逆算する。
    const float advectionSpeed = 1.0f;
    const float dt = g_deltaTime * advectionSpeed;
    const float2 previousUV = uv - velocity * dt / g_terrainSize;
    float4 fog = g_previousFog.SampleLevel(g_linearClamp, previousUV, 0.0f);

    // RG はワールド座標単位のノイズオフセットとして更新する。
    static const float OffsetRecovery = 0.1f;
    fog.rg = (fog.rg - velocity * dt) * exp(-OffsetRecovery * dt);

    // 正の発散では薄く、負の発散では濃くする。
    static const float DivergenceResponse = 0.75f;
    fog.b *= exp(-divergence * DivergenceResponse * dt);

    // 衝撃後は元の密度へ徐々に回復する。
    static const float DensityRecovery = 0.35f;
    const float densityRecoveryRate = 1.0f - exp(-DensityRecovery * dt);
    fog.b = clamp(lerp(fog.b, 1.0f, densityRecoveryRate), 0.0f, 2.0f);

    // かき乱され具合を局所的な衝撃から更新し、時間で減衰させる。
    static const float DisturbanceDecay = 1.5f;
    fog.a *= exp(-DisturbanceDecay * dt);

    const float speed = length(velocity);
    const float stirring = smoothstep(2.5f, 6.0f, speed);
    fog.a = saturate(fog.a + stirring * 8.0f * dt);
    fog.a = max(fog.a, velocityState.a);

    g_nextFog[pixel] = fog;
}
