#pragma once
#include "TY/Immediate2D.h"
#include "TY/RenderTargetTexture.h"

namespace Race
{
    class RaceDrawUpscaler
    {
    public:
        RaceDrawUpscaler();

        void init(const RenderTargetTexture& renderTexture);

        Immediate2D::Texture upscale(float renderScale, bool fsrEnabled);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
