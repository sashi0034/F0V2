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

cbuffer PhongLight : register(b10)
{
    float3 g_lightDirection;
    float3 g_lightColor;
    float3 g_eyePosition;
    float3 g_ambientLight;
}

struct PSInput
{
    float4 position : SV_POSITION;
    // float3 normal: NORMAL;
    // float3 color : COLOR;
    // float2 uv : TEXCOORD0;
    float4 worldPosition : TEXCOORD1;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    const int gridResolution = 4; // 4x4 頂点
    const float gridSize = 3.0f; // グリッド幅 3.0

    // id から x, z のグリッド座標
    const int xOffset6[6] = {0, 1, 0, 1, 1, 0};
    const int zOffset6[6] = {0, 0, 1, 0, 1, 1};

    int quadId = id / 6;
    int vertexInQuad = id % 6;

    // 四角形のX,Zインデックス
    int quadX = quadId % (gridResolution - 1);
    int quadZ = quadId / (gridResolution - 1);

    // グリッド上の頂点X,Zインデックス
    int xIndex = quadX + xOffset6[vertexInQuad];
    int zIndex = quadZ + zOffset6[vertexInQuad];

    // [-1.5, +1.5] の範囲にマッピング
    float x = ((float)xIndex / (gridResolution - 1)) * gridSize - (gridSize * 0.5f);
    float z = ((float)zIndex / (gridResolution - 1)) * gridSize - (gridSize * 0.5f);

    // y 波形
    float y = sin(x * 3.0f) * cos(z * 3.0f);

    result.worldPosition = float4(x, y, z, 1.0f);

    result.position = result.worldPosition;
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 finalColor = float4(1, 1, 1, 1);
    finalColor.r = abs(input.worldPosition.y) * 10.0f;
    return float4(finalColor);
}
