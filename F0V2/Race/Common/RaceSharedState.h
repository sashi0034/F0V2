#pragma once
#include "AIRank.h"
#include "CourseData.h"
#include "CourseTextureKind.h"
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

        std::array<RenderTarget, CourseTextureCount> courseTextures{};

        bool isRaceStarted{};

        bool isRaceEnded{};

        AIRank aiRank{};

        RaceSharedState();

        [[nodiscard]]
        const RenderTarget& courseTexture(CourseTextureKind kind) const
        {
            return courseTextures[static_cast<int>(kind)];
        }
    };

    inline InlineComponent<RaceSharedState> g_sharedState{};
}
