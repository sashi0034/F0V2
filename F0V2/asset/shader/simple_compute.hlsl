RWStructuredBuffer<uint> g_buffer : register(u0);

cbuffer BufferInfo : register(b0)
{
    uint g_elementCount;
}

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    if (DTid.x < g_elementCount)
    {
        g_buffer[DTid.x] += 5;
    }
}
