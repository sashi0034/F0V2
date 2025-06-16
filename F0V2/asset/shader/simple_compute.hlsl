RWStructuredBuffer<uint> g_buffer : register(u0);

StructuredBuffer<uint> g_readonlyData0 : register(t0);

StructuredBuffer<uint> g_readonlyData1 : register(t1);

cbuffer BufferInfo : register(b0)
{
    uint g_writableBufferSize[8];
    uint g_readonlyBufferSize[8];
}

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < g_writableBufferSize[0])
    {
        g_buffer[DTid.x] += g_readonlyData0[DTid.x / 2] + g_readonlyData1[DTid.x];
    }
}
