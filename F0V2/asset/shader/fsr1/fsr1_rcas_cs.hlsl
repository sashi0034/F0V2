// FidelityFX Super Resolution 1.0 - RCAS

#define A_GPU 1
#define A_HLSL 1
#define A_HALF 1

#include "ffx_a.h"

SamplerState g_sampler : register(s0); // linear clamp

Texture2D<AH4> g_inputTexture : register(t0);

RWTexture2D<AH4> g_outputTexture : register(u0);

#define FSR_RCAS_H 1

AH4 FsrRcasLoadH(ASW2 p)
{
    return g_inputTexture.Load(ASW3(ASW2(p), 0));
}

void FsrRcasInputH(inout AH1 r, inout AH1 g, inout AH1 b)
{
}

#include "ffx_fsr1.h"

// -----------------------------------------------

cbuffer RcasCB : register(b0)
{
    uint4 Const0;
};

void RcasFilter(ASU2 pos)
{
    AH3 c;
    FsrRcasH(c.r, c.g, c.b, pos, Const0);
    g_outputTexture[pos] = AH4(c, 1);
}

[numthreads(64, 1, 1)]
void CS(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID)
{
    AU2 gxy = ARmp8x8(LocalThreadId.x) + AU2(WorkGroupId.x << 4u, WorkGroupId.y << 4u);
    RcasFilter(gxy);
    gxy.x += 8u;
    RcasFilter(gxy);
    gxy.y += 8u;
    RcasFilter(gxy);
    gxy.x -= 8u;
    RcasFilter(gxy);
}
