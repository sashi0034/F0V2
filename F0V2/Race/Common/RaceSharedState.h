#pragma once
#include "CourseData.h"
#include "TY/Array.h"
#include "TY/InlineComponent.h"
#include "TY/RenderTarget.h"

namespace Race
{
    struct RaceSharedState : IInlineComponent
    {
        float groundPositionY = -50.0f;

        float fovFarZ = 1000.0f;

        Array<CourseSegment> courseSegments{};

        struct
        {
            UnorderedRenderTargetTexture albedo;
            UnorderedRenderTargetTexture normal;
            UnorderedRenderTargetTexture viewDistance;
        } gbuffer{};

        RenderTarget gbufferTarget{};

        RaceSharedState();
    };

    inline InlineComponent<RaceSharedState> g_sharedState{};
}
