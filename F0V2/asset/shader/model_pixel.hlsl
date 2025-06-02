#include "model.hlsli"

Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    const float z = input.position.z;

    const float3 texColor = g_texture0.Sample(g_sampler0, input.uv);
    // const float3 texColor = float3(input.uv.x, input.uv.y, 0);

    return float4(texColor * input.color * z, 1.0);
}
