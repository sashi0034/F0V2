// FidelityFX Super Resolution 1.0 - EASU

#define A_GPU 1
#define A_HLSL 1
#define A_HALF 1

#include "ffx_a.h"

SamplerState g_sampler : register(s0); // linear clamp

Texture2D<AH4> g_inputTexture : register(t0);

RWTexture2D<AH4> g_outputTexture : register(u0);

#define FSR_EASU_H 1

AH4 FsrEasuRH(AF2 p)
{
    return g_inputTexture.GatherRed(g_sampler, p, int2(0, 0));
}

AH4 FsrEasuGH(AF2 p)
{
    return g_inputTexture.GatherGreen(g_sampler, p, int2(0, 0));
}

AH4 FsrEasuBH(AF2 p)
{
    return g_inputTexture.GatherBlue(g_sampler, p, int2(0, 0));
}

#include "ffx_fsr1.h"

// -----------------------------------------------

cbuffer EasuCB : register(b0)
{
    uint4 Const0;
    uint4 Const1;
    uint4 Const2;
    uint4 Const3;
};

void EasuFilter(ASU2 pos)
{
    AH3 c;
    FsrEasuH(c, pos, Const0, Const1, Const2, Const3);
    g_outputTexture[pos] = AH4(c, 1);
}

[numthreads(64, 1, 1)]
void CS(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
    AU2 gxy = ARmp8x8(LocalThreadId.x) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
    EasuFilter(gxy);
    gxy.x += 8u;
    EasuFilter(gxy);
    gxy.y += 8u;
    EasuFilter(gxy);
    gxy.x -= 8u;
    EasuFilter(gxy);
}
