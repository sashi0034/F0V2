#include "basic.hlsli"

Texture2D<float4> g_texture0 : register(t0);

SamplerState g_sampler0 : register(s0);

float4 PS(PSInput input) : SV_TARGET
{
    return float4(g_texture0.Sample(g_sampler0, input.uv));
}
