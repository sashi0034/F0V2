#include "pch.h"
#include "MachineDrawer.h"

#include "Asset.generated.h"
#include "MachineConstants.h"
#include "Race/IRaceContext.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/InlineComponent.h"
#include "TY/ModelDrawer.h"
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

struct MachineDrawer::Impl
{
    MachineId m_id;
    ModelDrawer m_shadowDrawer{};
    ModelDrawer m_gbufferDrawer{};

    void Init(MachineId id, const ColorF32& linearColor)
    {
        m_id = id;

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
            .setShader(Asset_shader::gbuffer_pass);
    }

    void Update()
    {
        const auto& machine = GetRaceContext().machineManager().machineList()[m_id];

        Mat4x4 localRotation = Mat4x4(Quaternion::RotateX(Math::HalfPiF));
        if (machine.state.isDead())
        {
            localRotation = Mat4x4::Identity(); // TODO: 死亡グラフィック
        }

        const Mat4x4& worldMatrix = localRotation * machine.state.m_pose.getMatrix();

        (void)m_shadowDrawer.uploadWorldMatrix(worldMatrix);
        (void)m_gbufferDrawer.uploadWorldMatrix(worldMatrix);
    }
};

namespace Race
{
    MachineDrawer::MachineDrawer()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void MachineDrawer::init(MachineId id, const ColorF32& linearColor)
    {
        p_impl->Init(id, linearColor);
    }

    void MachineDrawer::update()
    {
        p_impl->Update();
    }

    void MachineDrawer::drawShadowMap() const
    {
        p_impl->m_shadowDrawer.draw();
    }

    void MachineDrawer::drawGBuffer() const
    {
        p_impl->m_gbufferDrawer.draw();
    }

    void MachineDrawer::drawTransparent() const
    {
    }
}
