RWStructuredBuffer<uint> g_buffer : register(u0);

RWStructuredBuffer<uint> g_readonlyData : register(u1);

cbuffer BufferInfo : register(b0)
{
    uint g_elementCount[8];
}

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < g_elementCount[0])
    {
        g_buffer[DTid.x] += g_readonlyData[DTid.x / 2];
    }
}
