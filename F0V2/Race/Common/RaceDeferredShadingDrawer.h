#pragma once
#include "TY/RenderTargetTexture.h"

namespace Race
{
    class RaceDeferredShadingDrawer
    {
    public:
        RaceDeferredShadingDrawer();

        void init();

        void draw(float renderScale) const;

        RenderTargetTexture getOutputTexture() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
