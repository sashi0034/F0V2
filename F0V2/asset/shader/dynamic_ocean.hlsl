static const float PI = 3.14159265359;

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
    float3 g_lightDirection;
    float3 g_eyePosition;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal: NORMAL;
    // float3 color : COLOR;
    // float2 uv : TEXCOORD0;
    float4 worldPosition : TEXCOORD1;
};

struct WaveElement
{
    float2 direction; // 波の進行方向（正規化）
    float wavelength; // 波長
    float amplitude; // 振幅
    float speed; // 波の速度
};

static const int WAVE_COUNT = 4;
static const WaveElement g_waves[WAVE_COUNT] = {
    {normalize(float2(1, 0)), 8.0f, 0.5f, 3.0f},
    {normalize(float2(1, -1)), 12.0f, 0.3f, 2.0f},
    {normalize(float2(-1, 2)), 5.0f, 0.2f, 5.0f},
    {normalize(float2(-1, -3)), 15.0f, 0.4f, 4.0f},
};

// ガースナー波
float3 gerstnerWavePosition(float3 pos, float time, out float3 normal)
{
    float3 newPos = pos;

    float3 n = float3(0, 0, 0); // 法線初期値

    const float gridStep = g_gridSize / (g_gridDensity - 1);

    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        WaveElement w = g_waves[i];
        float k = 2 * PI / w.wavelength;
        float a = w.amplitude;
        const float q = (2.0f) * gridStep / WAVE_COUNT; // TODO: 急峻さの部分をパラメータ化

        float f = k * dot(w.direction, pos.xz) - w.speed * time;

        // 頂点の変位
        newPos.x += w.direction.x * (q * cos(f));
        newPos.y += a * sin(f);
        newPos.z += w.direction.y * (q * cos(f));

        // x, z 軸方向の接線ベクトルを計算 (変位を各軸方向に偏微分したものと軸の方向ベクトルを加算)
        float3 tangentX = float3(
            1 + w.direction.x * (-q * sin(f)) * (k * w.direction.x),
            a * cos(f) * (k * w.direction.x),
            w.direction.y * (-q * sin(f)) * (k * w.direction.x)
        );

        float3 tangentZ = float3(
            w.direction.x * (-q * sin(f)) * (k * w.direction.y),
            a * cos(f) * (k * w.direction.y),
            1 + w.direction.y * (-q * sin(f)) * (k * w.direction.y)
        );

        // 接線ベクトルの外積を累積
        n += cross(tangentZ, tangentX);
    }

    normal = normalize(n);
    return newPos;
}

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

    float3 basePos = float3(x, 0, z);
    float3 normal;
    float3 displacedPos = gerstnerWavePosition(basePos, g_time, normal);

    result.worldPosition = float4(displacedPos, 1.0f);

    result.position = result.worldPosition;
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normal;

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    return float4(input.normal, 1);

    const float3 ambientColor = float4(0.1f, 0.1f, 0.2f, 1.0f);

    const float3 waterDiffuse = float3(0.3, 1, 1);
    const float3 diffuseColor = waterDiffuse * max(dot(input.normal, -float3(0.3, -1, 0.3)), 0.0f);

    // const float3 viewDirection = normalize(g_eyePosition - input.worldPosition.xyz);
    // const float3 reflectDirection = reflect(g_lightDirection, input.normal);
    // const float specularStrength = pow(max(dot(viewDirection, reflectDirection), 0.0f), g_shininess);
    // const float3 specularColor = float3(0.9, 0.9, 0.9) * specularStrength;

    const float3 finalColor = ambientColor + diffuseColor;
    return float4(finalColor, 1.0f);
}
