#pragma once
#include "TY/Color.h"
#include "TY/ModelDrawer.h"

namespace Race
{
    class MachineDrawer
    {
    public:
        MachineDrawer() = default;

        MachineDrawer(const ColorF32& linearColor);

        void uploadWorldMatrix(const Mat4x4& worldMatrix) const;

        void drawGBuffer() const;

    private:
        ModelDrawer m_gbufferDrawer{};
    };
}
