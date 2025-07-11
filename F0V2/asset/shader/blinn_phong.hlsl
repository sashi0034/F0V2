Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

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

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    float3 color : COLOR;
    float2 uv : TEXCOORD0;
    float4 worldPosition : TEXCOORD1;
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
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    const float3 N = normalize(input.normal);
    const float3 L = normalize(-g_lightDirection); // ライト方向
    const float3 V = normalize(g_eyePosition - input.worldPosition.xyz); // カメラ方向

    // 拡散反射
    float diff = max(dot(N, L), 0.0);

    // Blinn-Phong ハーフベクトル
    const float3 H = normalize(L + V);

    // 鏡面反射 (ハーフベクトルと法線のドットのべき乗)
    float spec = 0.0;
    if (diff > 0.0)
    {
        const float specPower = 5.0;
        spec = pow(max(dot(N, H), 0.0), specPower);
    }

    const float3 diffuseLight = g_lightColor * diff;
    const float3 specularLight = g_lightColor * spec;

    const float4 texColor = g_texture0.Sample(g_sampler0, input.uv) * float4(input.color, 1.0f);

    // 最終色計算 (環境光も加える)
    const float3 ambient = g_ambientLight;

    const float3 finalColor = texColor.xyz * (diffuseLight + specularLight + ambient);

    return float4(finalColor, texColor.a);
}
