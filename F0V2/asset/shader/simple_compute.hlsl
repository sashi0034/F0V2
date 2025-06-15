RWStructuredBuffer<uint> g_resultBuffer : register(u0);

[numthreads(64, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    // g_resultBuffer[DTid.x] += 5;
    g_resultBuffer[DTid.x] = 5;
}
