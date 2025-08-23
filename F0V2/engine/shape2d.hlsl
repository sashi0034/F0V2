Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
};

cbuffer ShapeDrawer : register(b0)
{
    row_major float2x4 g_transform;
    float4 g_colorMul;
    float4 g_colorAdd;
}

float4 transform2D(float2 pos, float2x4 t)
{
    return float4((t._13_14 + (pos.x * t._11_12) + (pos.y * t._21_22)), t._23_24);
}

PSInput VS(float2 position : POSITION, float2 uv : TEXCOORD, float4 color : COLOR0)
{
    PSInput result;

    result.position = transform2D(position, g_transform);
    result.uv = uv;
    result.color = color * g_colorMul;

    return result;
}

float4 PS_Shape(PSInput input) : SV_TARGET
{
    return input.color + g_colorAdd;
}

float4 PS_SquareDot(PSInput input) : SV_TARGET
{
    float tr = input.uv.y;

    float d = abs(fmod(input.uv.x, 3.0) - 1.0);

    float range = 1.0 - tr;

    input.color.a *= (d < range) ? 1.0 : (d < 1.0) ? ((1.0 - d) / tr) : 0.0;

    return (input.color + g_colorAdd);
}

float4 PS_RoundDot(PSInput input) : SV_TARGET
{
    float t = fmod(input.uv.x, 2.0);

    input.uv.x = abs(1 - t) * 2.0;

    float dist = dot(input.uv, input.uv) * 0.5;
    float delta = fwidth(dist);
    float alpha = smoothstep(0.5 - delta, 0.5, dist);
    input.color.a *= 1.0 - alpha;

    return (input.color + g_colorAdd);
}

float4 PS_BitmapFont(PSInput input) : SV_TARGET
{
    const float textAlpha = g_texture0.Sample(g_sampler0, input.uv).r;

    input.color.a *= textAlpha;

    return (input.color + g_colorAdd);
}
