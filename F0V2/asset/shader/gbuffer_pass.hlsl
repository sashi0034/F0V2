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

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float viewDistance : TEXCOORD0; // [near, far]
    float2 uv : TEXCOORD1;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);
    result.position = mul(g_viewMatrix, result.position);

    result.viewDistance = length(result.position);

    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normalize(mul((float3x3)g_worldMatrix, normal.xyz));

    result.uv = uv;

    return result;
}

struct PSOutput
{
    float4 albedoBuffer : SV_TARGET0;
    float4 normalBuffer : SV_TARGET1;
    float viewDistanceBuffer : SV_TARGET2;
};

PSOutput PS(PSInput input)
{
    PSOutput output;

    output.albedoBuffer = float4(g_texture0.Sample(g_sampler0, input.uv).rgb * g_diffuse, 1.0);

    output.normalBuffer = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);

    output.viewDistanceBuffer = input.viewDistance;

    return output;
}
