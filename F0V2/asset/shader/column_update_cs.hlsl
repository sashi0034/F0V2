Texture2D<float2> g_baseColumnTexture : register(t0);
Texture2D<float4> g_fogTexture : register(t1);

RWTexture2D<float2> g_outputColumnTexture : register(u0);

SamplerState g_linearWrap : register(s0);

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

    // 移流した RG をノイズ座標オフセットとして Column Texture を更新する。
    float2 column = g_baseColumnTexture.SampleLevel(g_linearWrap, uv + fog.rg, 0.0f);
    column.y *= fog.b;

    g_outputColumnTexture[pixel] = column;
}
