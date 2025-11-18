#include "pch.h"
#include "MachineEffectDrawer.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "TY/Array.h"
#include "TY/ConstantBufferArray.h"
#include "TY/GenericModelBuffer.h"
#include "TY/GenericModelDrawer.h"
#include "TY/StructuredBufferWrapper.h"

using namespace Race;

constexpr int particleCount = 10; // TODO

struct EffectModelBuffer : IGenericModelBuffer
{
    GenericModelShapeBufferElement m_shape{};

    EffectModelBuffer()
    {
        m_shape.materialIndex = 0;
        m_shape.indexBuffer = IndexBuffer::Placeholder(6 * particleCount);
    }

    int shapeCount() const override
    {
        return 1; // Assuming a single shape
    }

    GenericModelShapeBufferElement shapeAt(int index) const override
    {
        return m_shape;
    }

    int materialCount() const override
    {
        return 1; // Assuming a single material for the shape
    }

    ConstantBufferArrayImpl materialCbv() const override
    {
        return {Empty};
    }

    Array<Array<ShaderResourceType>> materialSrv() const override
    {
        return {};
    }
};

namespace
{
    struct ParticleElement
    {
        Float3 worldPos;
    };

    struct SimpleParticle_b10
    {
        Float3 cameraUp;
        float padding0;
        Float3 cameraRight;
    };
}

struct MachineEffectDrawer::Impl : IRaceDrawer
{
    GenericModelDrawer m_particleDrawer{};

    StructuredBufferWrapper<ParticleElement> m_particleBuffer{particleCount};

    ConstantBufferWrapper<SimpleParticle_b10> m_particleCB{};

    void Init()
    {
        m_particleDrawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(std::make_shared<EffectModelBuffer>())
            .setVertexInput({})
            .setOptions(
                GraphicsOptions()
                .setBlend(GraphicsBlendOptions::AlphaBlend())
                .setDepth(
                    GraphicsDepthOptions()
                    .setTestEnabled(true)
                    .setWriteMask(false))
            )
            .setShader(Asset_shader::simple_particle)
            .setCbv10AndLater({m_particleCB})
            .setSrv10AndLater({Asset_image::particle.fetchResource(), m_particleBuffer})
        };
    }

    void Update()
    {
        const MachinePhysicsUnit& machine = GetRaceContext().machineManager().machineList()[PlayerMachineId]; // TODO
        auto& camera = GetRaceContextContent().camera;

        m_particleCB->cameraUp = camera.worldMatrix().up();
        m_particleCB->cameraRight = camera.worldMatrix().right();
        m_particleCB.upload();

        for (int i = 0; i < m_particleBuffer.count(); ++i)
        {
            m_particleBuffer[i].worldPos = machine.state.m_pose.position + Float3{0, 0, 1.0f * i}; // TODO
        }

        m_particleBuffer.upload();
    }

    void drawTransparent() const override
    {
        m_particleDrawer.draw();
    }
};

namespace Race
{
    MachineEffectDrawer::MachineEffectDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void MachineEffectDrawer::init()
    {
        p_impl->Init();
        GetRaceContext().registerDrawer(p_impl);
    }

    void MachineEffectDrawer::finalize()
    {
        GetRaceContext().unregisterDrawer(p_impl.get());
    }

    void MachineEffectDrawer::update()
    {
        p_impl->Update();
    }
}
