Texture2D<float4> g_src : register(t0);
RWTexture2D<float4> g_dst : register(u0);
SamplerState g_sampler : register(s0);

[numthreads(8, 8, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    uint w, h;
    g_dst.GetDimensions(w, h);
    if (DTid.x >= w || DTid.y >= h) return;

    const float2 uv = (DTid.xy + 0.5) / float2(w, h);
    g_dst[DTid.xy] = g_src.SampleLevel(g_sampler, uv, 0); // bilinear 2x2
}
