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

cbuffer DynamicOcean : register(b10)
{
    int g_gridDensity;
    float g_gridSize;
    float g_time;
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

    // id から x, z のグリッド座標
    const int xOffset6[6] = {0, 1, 0, 1, 1, 0};
    const int zOffset6[6] = {0, 0, 1, 0, 1, 1};

    int quadId = id / 6;
    int vertexInQuad = id % 6;

    // 四角形の X,Z インデックス
    int quadX = quadId % (g_gridDensity - 1);
    int quadZ = quadId / (g_gridDensity - 1);

    // グリッド上の頂点 X, Z インデックス
    int xIndex = quadX + xOffset6[vertexInQuad];
    int zIndex = quadZ + zOffset6[vertexInQuad];

    // 平面にマッピング
    float x = ((float)xIndex / (g_gridDensity - 1)) * g_gridSize - (g_gridSize * 0.5f);
    float z = ((float)zIndex / (g_gridDensity - 1)) * g_gridSize - (g_gridSize * 0.5f);

    // y 波形
    float y = sin((x + g_time) * 3.0f) * cos((z + g_time) * 3.0f);

    result.worldPosition = float4(x, y, z, 1.0f);

    result.position = result.worldPosition;
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    float4 finalColor = float4(1, 1, 1, 1);
    finalColor.r = abs(input.worldPosition.y);
    return float4(finalColor);
}
