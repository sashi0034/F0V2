Texture2D<float4> g_texture0 : register(t10);

SamplerState g_sampler0 : register(s0);

cbuffer CheapAA_b10 : register(b10)
{
    float2 g_outputResolution;
}

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

PSInput VS(uint id : SV_VertexID)
{
    PSInput result;

    static const float2 pos[6] = {
        float2(-1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, -1.0),
        float2(1.0, -1.0),
        float2(-1.0, 1.0),
        float2(1.0, 1.0)
    };

    static const float2 uv[6] = {
        float2(0.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 1.0),
        float2(1.0, 1.0),
        float2(0.0, 0.0),
        float2(1.0, 0.0)
    };

    result.position = float4(pos[id], 0.0, 1.0);
    result.uv = uv[id];
    return result;
}

// -----------------------------------------------
float Luma(float3 c)
{
    // 一般的な輝度近似
    return dot(c, float3(0.299, 0.587, 0.114));
}

float4 PS(PSInput input) : SV_TARGET
{
    float2 rtInv = 1.0 / g_outputResolution;
    float2 uv = input.uv;

    // 中心
    float3 rgbM = g_texture0.Sample(g_sampler0, uv).rgb;
    float lumaM = Luma(rgbM);

    // 4 近傍
    float3 rgbN = g_texture0.Sample(g_sampler0, uv + float2(0, -rtInv.y)).rgb;
    float3 rgbS = g_texture0.Sample(g_sampler0, uv + float2(0, rtInv.y)).rgb;
    float3 rgbW = g_texture0.Sample(g_sampler0, uv + float2(-rtInv.x, 0)).rgb;
    float3 rgbE = g_texture0.Sample(g_sampler0, uv + float2(rtInv.x, 0)).rgb;

    float lumaN = Luma(rgbN);
    float lumaS = Luma(rgbS);
    float lumaW = Luma(rgbW);
    float lumaE = Luma(rgbE);

    // ローカルコントラスト
    float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaW, lumaE)));
    float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaW, lumaE)));
    float lumaRange = lumaMax - lumaMin;

    // エッジがほとんどないところは何もしない
    const float LUMA_THRESHOLD = 0.05; // 調整用
    if (lumaRange < LUMA_THRESHOLD)
    {
        return float4(rgbM, 1.0);
    }

    // 勾配 (gradient) を計算
    // x 方向：W - E, y 方向：N - S
    float2 grad = float2(lumaW - lumaE, lumaN - lumaS);

    // 勾配が小さすぎる場合も無視
    float gradMag = abs(grad.x) + abs(grad.y); // L1 ノルムで十分
    if (gradMag < 1e-4)
    {
        return float4(rgbM, 1.0);
    }

    // エッジ方向 = 勾配の垂直方向
    float2 dir = float2(-grad.y, grad.x);

    // 正規化 (ざっくりで OK)
    float dirLen = max(abs(dir.x), abs(dir.y)); // cheaply normalize
    dir /= (dirLen + 1e-4);

    // テクセル座標系にスケール（だいたい 0.5 ピクセル分）
    dir *= rtInv * 0.5;

    // エッジ方向に 2tap サンプル
    float3 rgbA = g_texture0.Sample(g_sampler0, uv + dir).rgb;
    float3 rgbB = g_texture0.Sample(g_sampler0, uv - dir).rgb;
    float3 edgeAvg = 0.5 * (rgbA + rgbB);

    // 中心とエッジ平均をブレンド
    // strength はお好みで（0.0, 1.0）
    const float STRENGTH = 0.6;
    float3 candidate = lerp(rgbM, edgeAvg, STRENGTH);

    // FXAA っぽく、局所輝度範囲から外れたら元色に戻す
    float lumaC = Luma(candidate);
    if (lumaC < lumaMin || lumaC > lumaMax)
    {
        candidate = rgbM;
    }

    return float4(candidate, 1.0);
}
