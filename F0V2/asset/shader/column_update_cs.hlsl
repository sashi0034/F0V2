Texture2D<float2> g_columnNoiseTexture : register(t0);
Texture2D<float4> g_fogTexture : register(t1);

RWTexture2D<float2> g_outputColumnTexture : register(u0);

SamplerState g_linearWrap : register(s0);

cbuffer ColumnUpdateParams : register(b0)
{
    float g_terrainSize;
    float3 g_padding;
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixel = dispatchThreadID.xy;

    uint width;
    uint height;
    g_outputColumnTexture.GetDimensions(width, height);
    if (pixel.x >= width || pixel.y >= height)
    {
        return;
    }

    const float2 resolution = float2(width, height);
    const float2 uv = (float2(pixel) + 0.5f) / resolution;
    const float4 fog = g_fogTexture.Load(int3(pixel, 0));

    // RG はワールド座標単位なので、ノイズ座標へ加えてから UV へ変換する。
    const float2 worldPosition = (uv - 0.5f) * g_terrainSize;
    const float2 noisePosition = worldPosition + fog.rg;
    const float2 noiseUV = noisePosition / g_terrainSize + 0.5f;

    const float v = g_columnNoiseTexture.SampleLevel(g_linearWrap, uv, 0.0f).r;
    const float n = g_columnNoiseTexture.SampleLevel(g_linearWrap, noiseUV, 0.0f).g;
    const float fogHeight = 1.4f * v * n;
    const float fogWeight = fogHeight * 0.8f * fog.b;

    g_outputColumnTexture[pixel] = float2(fogHeight, fogWeight);
}
