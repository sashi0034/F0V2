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
    {normalize(float2(2, 1)), 8.0f, 0.2f, 3.0f},
    {normalize(float2(1, -1)), 12.0f, 0.3f, 2.0f},
    {normalize(float2(-1, 2)), 5.0f, 0.2f, 5.0f},
    {normalize(float2(-1, -3)), 15.0f, 0.3f, 4.0f},
};

// ガースナー波
// https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models
float3 gerstnerWavePosition(float3 pos, out float3 normal)
{
    float3 newPos = pos;

    float3 n = float3(0, 1, 0);

    for (int i = 0; i < WAVE_COUNT; ++i)
    {
        const WaveElement wave = g_waves[i];

        const float w = 2 * PI / wave.wavelength;
        const float a = wave.amplitude;
        const float wa = w * a;
        const float2 dir = wave.direction;
        const float q = (0.5 / WAVE_COUNT) / a; // TODO: 急峻さの部分をパラメータ化

        const float phase = w * dot(dir, pos.xz) - wave.speed * g_time;

        // 変位の計算
        newPos.x += q * a * dir.x * cos(phase);
        newPos.y += a * sin(phase);
        newPos.z += q * a * dir.y * cos(phase);

        // 法線の計算
        n.x += -dir.x * wa * cos(phase);
        n.y += -q * wa * sin(phase);
        n.z += -dir.y * wa * cos(phase);
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

    float3 basePos = float3(x, 0, z) + g_worldMatrix._14_24_34;
    float3 normal;
    float3 displacedPos = gerstnerWavePosition(basePos, normal);

    result.worldPosition = float4(displacedPos, 1.0f);

    result.position = result.worldPosition;
    result.position = mul(g_viewMatrix, result.position);
    result.position = mul(g_projectionMatrix, result.position);

    result.normal = normal;

    return result;
}

float4 PS(PSInput input) : SV_TARGET
{
    // return float4(input.normal, 1.0f);

    float3 N = normalize(input.normal);
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

    const float3 diffuseColor = float3(0.2, 0.3, 0.3);
    const float3 specularColor = float3(0.2f, 0.3f, 0.3f);

    const float3 diffuseOutput = diffuseColor * diff;
    const float3 specularOutput = specularColor * spec;

    const float3 ambientColor = float3(0.1f, 0.1f, 0.2f);

    const float3 finalColor = (diffuseOutput + specularOutput + ambientColor);

    return float4(finalColor, 1.0);
}
