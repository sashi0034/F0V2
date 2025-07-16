#define SHADOW_MAP_COUNT 3

Texture2D<float4> g_texture0 : register(t0);

Texture2D<float4> g_shadowMapTexture : register(t1);

SamplerState g_sampler0 : register(s0);

SamplerComparisonState g_shadowMapSampler : register(s1);

cbuffer SceneState : register(b0)
{
    float4x4 g_projectionMatrix;
    float4x4 g_viewMatrix;
}

cbuffer ModelState : register(b1)
{
    float4x4 g_worldMatrix;
}

cbuffer ModelMaterial : register(b2)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

cbuffer PhongLight : register(b4)
{
    float3 g_lightDirection;
    float3 g_lightColor;
    float3 g_eyePosition;
    float3 g_ambientLight;
}

cbuffer ShadowMap : register(b5)
{
    float4x4 g_worldToShadowProjection[SHADOW_MAP_COUNT];
}

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
    float4 worldPosition : TEXCOORD1;
    float4 shadowPosition[SHADOW_MAP_COUNT] : TEXCOORD2;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.worldPosition = mul(g_worldMatrix, position);

    result.position = result.worldPosition;
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normalize(mul(g_worldMatrix, normal.xyz));

    result.color = g_diffuse;

    result.uv = uv;

    for (int i = 0; i < SHADOW_MAP_COUNT; ++i)
    {
        result.shadowPosition[i] = mul(g_worldToShadowProjection[i], result.worldPosition);
    }

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float t1 = dot(input.normal, g_lightDirection);
    t1 *= -1.0f;
    t1 = max(t1, 0.0f);

    const float3 diffuseLight = g_lightColor * t1;

    const float3 reflectVector = reflect(g_lightDirection, input.normal);

    // 光が当たった物体の表面から視線へ伸びるベクトル
    const float3 toEye = normalize(g_eyePosition - input.worldPosition.xyz);

    float t2 = dot(reflectVector, toEye);
    t2 = max(t2, 0.0f);
    t2 = pow(t2, 5.0f); // 鏡面反射の強さを強める

    float3 specularLight = g_lightColor * t2;

    float4 finalColor = g_texture0.Sample(g_sampler0, input.uv) * float4(input.color, 1.0f);

    // 乗算して最終的な色を求める
    finalColor.xyz *= (diffuseLight.xyz + specularLight.xyz + g_ambientLight);

    // 影
    for (int i = 0; i < SHADOW_MAP_COUNT; ++i)
    {
        float2 shadowUV = input.shadowPosition[i].xy / input.shadowPosition[i].w;
        shadowUV = shadowUV * float2(0.5f, -0.5f) + float2(0.5f, 0.5f); // [-1, 1] -> [0, 1]

        if (0.0 <= shadowUV.x && shadowUV.x <= 1.0 && 0.0 <= shadowUV.y && shadowUV.y <= 1.0)
        {
            // 影マップの範囲内
            const float shadowZ = input.shadowPosition[i].z / input.shadowPosition[i].w;
            const float shadowValue = g_shadowMapTexture.SampleCmpLevelZero(g_shadowMapSampler, shadowUV, shadowZ);
            finalColor.xyz *= lerp(1.0f, 0.5f, shadowValue);
            break;
        }
    }

    // 影の反映

    return finalColor;
}
