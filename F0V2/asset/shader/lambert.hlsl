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

cbuffer LambertLight : register(b10)
{
    float3 g_lightDirection;
    float3 g_lightColor;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    float2 uv : TEXCOORD;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMatrix, position);
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normalize(mul((float3x3)g_worldMatrix, normal.xyz));

    result.uv = uv;
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float t = dot(input.normal, g_lightDirection);
    t *= -1.0f;
    if (t < 0.0f)
    {
        t = 0.0f;
    }

    const float3 diffuseLight = g_lightColor * t;

    float4 finalColor = g_texture0.Sample(g_sampler0, input.uv) * float4(g_diffuse, 1.0f);

    finalColor.xyz *= diffuseLight.xyz;

    return finalColor;
}
