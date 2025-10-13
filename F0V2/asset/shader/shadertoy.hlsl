Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

// cbuffer SceneState : register(b0)
// {
//     float4x4 g_projectionMatrix;
//     float4x4 g_viewMatrix;
// }

cbuffer Shadertoy_b10 : register(b10)
{
    float2 g_screenResolution;
    float2 g_mousePosition;
    float2 g_jitterOffset;
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

float4 PS(PSInput input) : SV_TARGET
{
    float4 color = float4(0, 0, 0, 1);

    float2 inputPosition = input.position.xy + g_jitterOffset;

    float2 pos = inputPosition / g_screenResolution;

    color.r = (pos.x + 0.5) * 0.5;
    color.g = (pos.y + 0.5) * 0.5;

    if (length(g_mousePosition - inputPosition) < 100)
    {
        color.b = 1;
    }

    // if ((input.position.x + input.position.y) % 2 == 0)
    // {
    //     color.rgb = float3(1, 0.1, 0);
    // }
    // else
    // {
    //     color.rgb = float3(0, 0.1, 1);
    // }

    return color;
}
