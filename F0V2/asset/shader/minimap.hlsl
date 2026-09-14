Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

cbuffer SceneState : register(b0)
{
    column_major float4x4 g_projectionMatrix;
    column_major float4x4 g_viewMatrix;
}

cbuffer ModelState : register(b1)
{
    column_major float4x4 g_worldMatrix;
}

cbuffer ModelMaterial : register(b2)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

// ミニマップは単色 + 簡易陰影で描くので、マテリアルもテクスチャも参照しない
static const float3 g_minimapLightDirection = normalize(float3(0.3, -1.0, 0.4));

static const float3 g_minimapBaseColor = float3(0.5, 0.5, 0.5);

static const float g_minimapAmbient = 0.35;

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normalize(mul((float3x3)g_worldMatrix, normal.xyz));

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    const float3 n = normalize(input.normal);
    const float ndl = saturate(dot(n, -g_minimapLightDirection));

    const float shade = g_minimapAmbient + (1.0 - g_minimapAmbient) * ndl;

    return float4(g_minimapBaseColor * shade, 1.0);
}
