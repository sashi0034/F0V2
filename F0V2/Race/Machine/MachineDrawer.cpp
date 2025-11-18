#include "pch.h"
#include "MachineDrawer.h"

#include "Asset.generated.h"
#include "MachineConstants.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/InlineComponent.h"
#include "TY/PrimitiveModel3D.h"

using namespace Race;

namespace
{
    struct MachineDrawerCache : IInlineComponent
    {
        ModelData modelData = PrimitiveModel3D::Capsule(MachineRadius, MachineHeight, ColorF32{1.0f});

        ModelShapeBuffer shapeBuffer = ModelShapeBuffer{modelData.shapes};
    };

    InlineComponent<MachineDrawerCache> s_machineDrawerCache{};
}

namespace Race
{
    MachineDrawer::MachineDrawer(const ColorF32& linearColor)
    {
        auto materials = s_machineDrawerCache->modelData.materials;
        materials[0].parameters.diffuse = linearColor.toFloat3();

        const ModelBuffer model = ModelBuffer{s_machineDrawerCache->shapeBuffer, materials};

        m_shadowDrawer =
            ModelDrawerParams{}
            .setModel(model)
            .setOptions(GraphicsOptions::FromTarget(g_sharedState->shadowMap))
            .setShader(Asset_shader::shadow_caster)
            .setCbv10AndLater({g_sharedState->cb.shadowCaster});

        m_gbufferDrawer =
            ModelDrawerParams{}
            .setModel(model)
            .setOptions(GraphicsOptions::FromTarget(g_sharedState->gbufferTarget))
            .setShader(Asset_shader::gbuffer_pass);;
    }

    void MachineDrawer::uploadWorldMatrix(const Mat4x4& worldMatrix) const
    {
        (void)m_shadowDrawer.uploadWorldMatrix(worldMatrix);
        (void)m_gbufferDrawer.uploadWorldMatrix(worldMatrix);
    }

    void MachineDrawer::drawShadowMap() const
    {
        m_shadowDrawer.draw();
    }

    void MachineDrawer::drawGBuffer() const
    {
        m_gbufferDrawer.draw();
    }

    void MachineDrawer::drawTransparent() const
    {
        m_machineEffectDrawer.drawTransparent();
    }
}
