RWStructuredBuffer<uint> g_buffer : register(u0);

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    g_buffer[DTid.x] += 5;
}
