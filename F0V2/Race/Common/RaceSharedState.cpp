#include "pch.h"
#include "RaceSharedState.h"

#include "TY/Screen.h"

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
            .setClearColor(ColorF32{fovFarZ, 0.0f, 0.0f, 0.0f}); // FIXME?

        gbufferTarget =
            RenderTargetParams{}
            .setTargetList({
                gbuffer.albedo,
                gbuffer.normal,
                gbuffer.viewDistance
            });
    }
}
