RWStructuredBuffer<uint> g_buffer : register(u0);

StructuredBuffer<uint> g_readonlyData0 : register(t0);

StructuredBuffer<uint> g_readonlyData1 : register(t1);

cbuffer BufferInfo : register(b0)
{
    uint3 g_t0_size;
    uint3 g_u0_size;
}

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < g_u0_size.x)
    {
        g_buffer[DTid.x] += g_readonlyData0[DTid.x / 2] + g_readonlyData1[DTid.x];
    }
}
