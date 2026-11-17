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
    const float2 velocity = g_velocityField.SampleLevel(g_linearClamp, uv, 0.0f).rg;

    // Semi-Lagrangian 法で前フレームの位置を逆算する。
    const float advectionSpeed = 1.0f;
    const float dt = g_deltaTime * advectionSpeed;
    const float2 previousUV = uv - velocity * dt / g_terrainSize;
    float4 fog = g_previousFog.SampleLevel(g_linearClamp, previousUV, 0.0f);

    // RG はノイズ座標のオフセットとして速度に応じて更新する。
    fog.rg += velocity * g_deltaTime / g_terrainSize;
    fog.rg = frac(fog.rg + 0.5f) - 0.5f;

    fog.b = saturate(fog.b);
    fog.a = saturate(fog.a + length(velocity) * g_deltaTime * 0.05f);

    g_nextFog[pixel] = fog;
}
