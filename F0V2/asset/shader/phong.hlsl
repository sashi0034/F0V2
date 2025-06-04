Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

cbuffer SceneState : register(b0)
{
    float4x4 g_worldMat;
    float4x4 g_viewMat;
    float4x4 g_projectionMat;
}

cbuffer ModelMaterial : register(b1)
{
    float3 g_ambient;
    float3 g_diffuse;
    float3 g_specular;
    float g_shininess;
}

cbuffer Phong : register(b2)
{
    float3 g_lightDirection;
    float3 g_lightColor;
    float3 g_eyePosition;
    float3 g_ambientLight;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD;
};

PSInput VS(float4 position : POSITION, float4 normal : NORMAL, float2 uv : TEXCOORD)
{
    PSInput result;

    result.position = mul(g_worldMat, position);
    result.position = mul(g_viewMat, result.position);
    result.position = mul(g_projectionMat, result.position);

    result.normal = normal;

    result.color = g_diffuse;

    result.uv = uv;
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
    const float3 toEye = normalize(g_eyePosition - input.position.xyz);

    float t2 = dot(reflectVector, toEye);
    t2 = max(t2, 0.0f);
    t2 = pow(t2, 5.0f); // 鏡面反射の強さを強める

    float3 specularLight = g_lightColor * t2;

    float4 finalColor = g_texture0.Sample(g_sampler0, input.uv) * float4(input.color, 1.0f);

    // 乗算して最終的な色を求める
    finalColor.xyz *= (diffuseLight.xyz + specularLight.xyz + g_ambientLight);

    return finalColor;
}
