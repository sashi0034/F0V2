#pragma once
#include "MachineEffectDrawer.h"
#include "TY/Color.h"
#include "TY/ModelDrawer.h"

namespace Race
{
    class MachineDrawer
    {
    public:
        MachineDrawer() = default;

        MachineDrawer(const ColorF32& linearColor);

        void uploadWorldMatrix(const Mat4x4& worldMatrix);

        void drawShadowMap() const;

        void drawGBuffer() const;

        void drawTransparent() const;

    private:
        ModelDrawer m_shadowDrawer{};
        ModelDrawer m_gbufferDrawer{};
        MachineEffectDrawer m_machineEffectDrawer{};
    };
}
