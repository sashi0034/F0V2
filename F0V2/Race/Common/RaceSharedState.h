#pragma once
#include "AIRank.h"
#include "CourseData.h"
#include "CB/ShadowCaster.h"
#include "TY/Array.h"
#include "TY/DynamicHandle.h"
#include "TY/InlineComponent.h"
#include "TY/RenderTarget.h"

namespace Race
{
    struct RaceSharedState : IInlineComponent
    {
        float fov = 75.0_deg;

        float nearDepth = 0.1f;

        float farDepth = 500.0f;

        std::string coursePath{};

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
            CB::ShadowCaster_b10 shadowCaster{};
            DynamicCbvHandle shadowCasterCbv{};
        } cb{};

        RenderTarget shadowMap{};

        struct
        {
            RenderTarget boostPad{};
            RenderTarget jumpPad{};
            RenderTarget pitZone{};
        } gimmickTextures{};

        bool isRaceStarted{};

        bool isRaceEnded{};

        AIRank aiRank{};

        RaceSharedState();
    };

    inline InlineComponent<RaceSharedState> g_sharedState{};
}
