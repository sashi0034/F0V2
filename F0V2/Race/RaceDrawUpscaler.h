#pragma once
#include "TY/RenderTargetTexture.h"

namespace Race
{
    class RaceDrawUpscaler
    {
    public:
        RaceDrawUpscaler();

        void init(const RenderTargetTexture& renderTexture);

        TextureHandle upscale(float renderScale, bool fsrEnabled);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
