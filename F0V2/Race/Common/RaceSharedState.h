#pragma once
#include "AiRank.h"
#include "CourseData.h"
#include "CB/ShadowCaster.h"
#include "TY/Array.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/InlineComponent.h"
#include "TY/RenderTarget.h"

namespace Race
{
    struct RaceSharedState : IInlineComponent
    {
        float groundPositionY = -50.0f;

        float fov = 75.0_deg;

        float nearDepth = 0.1f;

        float farDepth = 1000.0f;

        Array<CourseSegment> courseSegments{};

        struct
        {
            UnorderedRenderTargetTexture albedo;
            UnorderedRenderTargetTexture normal;
            UnorderedRenderTargetTexture viewDistance;
        } gbuffer{};

        RenderTarget gbufferTarget{};

        struct
        {
            ConstantBufferWrapper<CB::ShadowCaster_b10> shadowCaster{};
        } cb{};

        RenderTarget shadowMap{};

        bool isRaceStarted{};

        bool isRaceEnded{};

        AiRank aiRank{};

        RaceSharedState();
    };

    inline InlineComponent<RaceSharedState> g_sharedState{};
}
