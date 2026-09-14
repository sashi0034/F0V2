Texture2D<float4> g_texture0 : register(t10);

static const float4 OutlineColor = float4(1.0, 1.0, 1.0, 1.0);

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

float4 PS(PSInput input) : SV_TARGET
{
    uint width;
    uint height;
    g_texture0.GetDimensions(width, height);
    const int2 maxIndex = int2(width, height) - 1;

    const int2 p = int2(input.position.xy);

    // 不透明な画素はコースの色をそのまま出す
    const float4 center = g_texture0.Load(int3(p, 0));
    if (center.a > 0.0) return center;

    // 透明な画素は、上下左右のどれかが不透明なら縁にする (太さ 1px)
    static const int2 neighbors[4] = {
        int2(1, 0),
        int2(-1, 0),
        int2(0, 1),
        int2(0, -1)
    };

    for (int i = 0; i < 4; ++i)
    {
        const int2 q = clamp(p + neighbors[i], int2(0, 0), maxIndex);
        if (g_texture0.Load(int3(q, 0)).a > 0.0) return OutlineColor;
    }

    return center;
}
