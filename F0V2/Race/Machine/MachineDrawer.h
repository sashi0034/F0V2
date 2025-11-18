#pragma once
#include "MachineEffectDrawer.h"
#include "TY/Color.h"
#include "TY/ModelDrawer.h"

namespace Race
{
    class MachineDrawer
    {
    public:
        MachineDrawer();

        void init(MachineId id, const ColorF32& linearColor);

        void update();

        void drawShadowMap() const;

        void drawGBuffer() const;

        void drawTransparent() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
