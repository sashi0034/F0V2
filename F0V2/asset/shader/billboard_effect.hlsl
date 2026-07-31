SamplerState g_sampler0 : register(s0);

Texture2D<float4> g_texture0 : register(t10);

struct ParticleElement
{
    float3 worldPos;
    float rotation;
    float2 size;
    float2 padding;
    float4 color;
};

StructuredBuffer<ParticleElement> g_particleBuffer : register(t11);

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

cbuffer BillboardParticle_b10 : register(b10)
{
    float3 g_cameraUp;
    float3 g_cameraRight;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    const ParticleElement particleElement = g_particleBuffer[id / 6];
    const uint vertId = id % 6;

    static const float2 k_offset[6] = {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    static const float2 k_uv[6] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0)
    };

    result.position = float4(particleElement.worldPos, 1.0);

    const float sine = sin(particleElement.rotation);
    const float cosine = cos(particleElement.rotation);
    const float2 rotatedOffset = float2(
        k_offset[vertId].x * cosine - k_offset[vertId].y * sine,
        k_offset[vertId].x * sine + k_offset[vertId].y * cosine
    );
    const float3 billboardOffset =
        g_cameraRight * rotatedOffset.x * particleElement.size.x +
        g_cameraUp * rotatedOffset.y * particleElement.size.y;
    result.position.xyz += billboardOffset;

    result.position = mul(g_worldMatrix, result.position);
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.uv = k_uv[vertId];
    result.color = particleElement.color;
    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 textureColor = g_texture0.Sample(g_sampler0, input.uv);
    textureColor *= input.color;
    return textureColor;
}
