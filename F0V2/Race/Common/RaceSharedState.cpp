#include "pch.h"
#include "RaceSharedState.h"

#include "TY/Screen.h"

using namespace Race;

namespace
{
    /// @brief kind ごとのテクスチャの一辺のサイズ
    int GetTextureSizeOf(CourseDynamicTextureKind kind)
    {
        switch (kind)
        {
        case CourseDynamicTextureKind::RoadTop: return 512;
        case CourseDynamicTextureKind::RoadBottom: return 256;
        case CourseDynamicTextureKind::RoadSide: return 256;
        case CourseDynamicTextureKind::BoostPad: return 128;
        case CourseDynamicTextureKind::JumpPad: return 128;
        case CourseDynamicTextureKind::PitZone: return 256;
        default: break;
        }

        assert(false);
        return 128;
    }
}

namespace Race
{
    RaceSharedState::RaceSharedState()
    {
        const Size gbufferSize = Screen::Size();

        gbuffer.albedo =
            RenderTargetTextureParams()
            .setSize(gbufferSize)
            .setFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
            .setClearColor(ColorF32{0.0f, 0.0f, 0.0f, 0.0f});

        gbuffer.normal =
            RenderTargetTextureParams()
            .setSize(gbufferSize)
            .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT) // TODO: DXGI_FORMAT_R10G10B10A2_UNORM?
            .setClearColor(ColorF32{0.0f, 0.0f, 0.0f, 0.0f});

        gbuffer.viewDistance =
            RenderTargetTextureParams()
            .setSize(gbufferSize)
            .setFormat(DXGI_FORMAT_R32_FLOAT)
            .setClearColor(ColorF32{farDepth, 0.0f, 0.0f, 0.0f}); // FIXME?

        gbufferTarget =
            RenderTargetParams{}
            .setRtvList({
                gbuffer.albedo,
                gbuffer.normal,
                gbuffer.viewDistance
            });

        // -----------------------------------------------

        constexpr Size shadowMapSize = Size{2048, 2048};

        shadowMap =
            RenderTargetParams()
            .setRtv(
                RtvParams{}
                .setSize(shadowMapSize)
                .setClearColor(ColorF32{1.0f, 1.0f}) // FIXME?
                .setFormat(DXGI_FORMAT_R32_FLOAT)
            );

        // -----------------------------------------------

        for (int i = 0; i < CourseDynamicTextureKindCount; ++i)
        {
            const int textureSize = GetTextureSizeOf(static_cast<CourseDynamicTextureKind>(i));

            courseDynamicTextures[i] =
                RenderTargetParams{}
                .setRtv(
                    RtvParams{}
                    .setSize(Size::One() * textureSize)
                    .setClearColor(ColorF32{1.0f, 1.0f}));
        }
    }
}
