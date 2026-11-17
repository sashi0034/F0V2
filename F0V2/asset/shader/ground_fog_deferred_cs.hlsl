Texture2D<float4> g_colorBuffer : register(t0);
Texture2D<float4> g_normalBuffer : register(t1);
Texture2D<float> g_depthBuffer : register(t2);

RWTexture2D<float4> g_output : register(u0);

cbuffer DeferredParams : register(b0)
{
    column_major float4x4 g_projectionMatrixInv;
    column_major float4x4 g_viewMatrixInv;
    float2 g_outputResolution;
    float2 g_padding;
}

// デプスバッファの値と画面座標からワールド座標を復元する。
float3 ReconstructWorldPosition(uint2 pixel, float depth)
{
    const float2 uv = (float2(pixel) + 0.5f) / g_outputResolution;
    const float2 ndcXY = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    const float4 clipPosition = float4(ndcXY, depth, 1.0f);

    float4 viewPosition = mul(g_projectionMatrixInv, clipPosition);
    viewPosition /= viewPosition.w;

    float4 worldPosition = mul(g_viewMatrixInv, float4(viewPosition.xyz, 1.0f));
    worldPosition /= worldPosition.w;
    return worldPosition.xyz;
}

float3 LinearToSRGB(float3 color)
{
    return lerp(
        color * 12.92f,
        1.055f * pow(max(color, 0.0f), 1.0f / 2.4f) - 0.055f,
        step(0.0031308f, color));
}

[numthreads(8, 8, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    const uint2 pixel = dispatchThreadID.xy;
    const uint2 resolution = uint2(g_outputResolution);
    if (pixel.x >= resolution.x || pixel.y >= resolution.y)
    {
        return;
    }

    const float depth = g_depthBuffer[pixel];

    // 深度 1.0 は地形が描画されていない背景。
    if (depth >= 1.0f)
    {
        const float skyGradient = 1.0f - float(pixel.y) / g_outputResolution.y;
        const float3 skyColor = lerp(float3(0.38f, 0.52f, 0.65f), float3(0.12f, 0.25f, 0.48f), skyGradient);
        g_output[pixel] = float4(skyColor, 1.0f);
        return;
    }

    const float3 worldPosition = ReconstructWorldPosition(pixel, depth);
    const float3 normal = normalize(g_normalBuffer[pixel].xyz * 2.0f - 1.0f);

    // ライトは仮実装としてシェーダー内に固定する。
    const float3 lightPosition = float3(-100.0f, 250.0f, -100.0f);
    const float3 lightDirection = normalize(lightPosition - worldPosition);
    const float diffuse = saturate(dot(normal, lightDirection));

    const float3 linearAlbedo = g_colorBuffer[pixel].rgb;

    const float3 ambientLight = float3(1, 1, 1) * 0.28f;
    const float3 lightColor = float3(1, 1, 1) * 0.72f;
    const float3 linearColor = linearAlbedo * (ambientLight + lightColor * diffuse);

    const float3 outputColor = LinearToSRGB(saturate(linearColor));

    g_output[pixel] = float4(outputColor, 1.0f);
}
