Texture2D<float4> g_colorBuffer : register(t0);
Texture2D<float4> g_normalBuffer : register(t1);
Texture2D<float> g_depthBuffer : register(t2);
Texture2D<float> g_terrainHeightMap : register(t3);
Texture2D<float2> g_columnTexture : register(t4);

SamplerState g_linearClamp : register(s0);

RWTexture2D<float4> g_output : register(u0);

cbuffer DeferredParams : register(b0)
{
    column_major float4x4 g_projectionMatrixInv; // NDC to View
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

    const float2 uv = (float2(pixel) + 0.5f) / g_outputResolution;
    const float2 ndcXY = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);

    float4 viewFar = mul(g_projectionMatrixInv, float4(ndcXY, 1.0f, 1.0f));
    viewFar /= viewFar.w;

    const float3 rayOrigin = mul(g_viewMatrixInv, float4(0.0f, 0.0f, 0.0f, 1.0f)).xyz;
    const float3 rayDirection = normalize(mul((float3x3)g_viewMatrixInv, viewFar.xyz));

    const float depth = g_depthBuffer[pixel];
    float maxDistance = 100.0f;
    float3 linearSceneColor;

    if (depth < 1.0f)
    {
        // GBuffer あり
        const float3 worldPosition = ReconstructWorldPosition(pixel, depth);
        maxDistance = min(maxDistance, length(worldPosition - rayOrigin));

        const float3 normal = normalize(g_normalBuffer[pixel].xyz * 2.0f - 1.0f);

        // ライトは仮実装としてシェーダー内に固定する。
        const float3 lightPosition = float3(0.0f, 250.0f, 0.0f);
        const float3 lightDirection = normalize(lightPosition - worldPosition);
        const float diffuse = saturate(dot(normal, lightDirection));

        const float3 ambientLight = float3(1.0f, 1.0f, 1.0f) * 0.28f;
        const float3 lightColor = float3(1.0f, 1.0f, 1.0f) * 0.72f;
        linearSceneColor = g_colorBuffer[pixel].rgb * (ambientLight + lightColor * diffuse);
    }
    else
    {
        // GBuffer なし (空)
        const float skyGradient = 1.0f - float(pixel.y) / g_outputResolution.y;
        linearSceneColor = lerp(
            float3(0.38f, 0.52f, 0.65f),
            float3(0.12f, 0.25f, 0.48f),
            skyGradient);
    }

    static const int FogSteps = 64;
    static const float TerrainSize = 100.0f;

    const float stepLength = maxDistance / FogSteps;
    float transmittance = 1.0f;

    // 霧のボリュームレンダリング
    for (int i = 0; i < FogSteps; ++i)
    {
        const float distance = (float(i) + 0.5f) * stepLength;
        const float3 samplePosition = rayOrigin + rayDirection * distance;
        const float2 terrainUV = samplePosition.xz / TerrainSize + 0.5f;

        float groundHeight;
        float2 fogColumn;
        if (any(terrainUV < 0.0f) || any(terrainUV > 1.0f))
        {
            // マップ範囲外
            groundHeight = -10.0f; // 海っぽく霧が広がってるイメージ
            fogColumn = float2(1.4f, 1.4f * 0.8f);
        }
        else
        {
            // ハイトマップとコラムマップからサンプルする
            groundHeight = g_terrainHeightMap.SampleLevel(g_linearClamp, terrainUV, 0.0f);
            fogColumn = g_columnTexture.SampleLevel(g_linearClamp, terrainUV, 0.0f);
        }

        const float fogHeight = fogColumn.x;
        const float fogWeight = fogColumn.y;
        const float heightFromGround = samplePosition.y - groundHeight;

        if (fogHeight <= 0.0001f || heightFromGround < 0.0f || heightFromGround >= fogHeight)
        {
            continue;
        }

        const float s = heightFromGround / fogHeight;
        const float falloff = 1.5f;
        const float opacity = 2.0f;

        const float density = fogWeight / fogHeight * (falloff + 1.0f) * pow(1.0f - s, falloff);

        transmittance *= exp(-opacity * density * stepLength);

        if (transmittance < 0.01f)
        {
            break;
        }
    }

    const float3 linearFogColor = float3(0.70f, 0.70f, 0.82f);
    const float3 linearOutput = lerp(linearFogColor, linearSceneColor, transmittance);
    const float3 outputColor = LinearToSRGB(saturate(linearOutput));

    g_output[pixel] = float4(outputColor, 1.0f);
}
