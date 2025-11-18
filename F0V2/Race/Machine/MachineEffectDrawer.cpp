#include "pch.h"
#include "MachineEffectDrawer.h"

#include "Asset.generated.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "TY/Array.h"
#include "TY/ConstantBufferArray.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBuffer.h"
#include "TY/GenericModelDrawer.h"
#include "TY/Periodic.h"
#include "TY/StructuredBufferWrapper.h"

using namespace Race;

constexpr int maxParticleCount = 1024;

struct EffectModelBuffer : IGenericModelBuffer
{
    GenericModelShapeBufferElement m_shape{};

    EffectModelBuffer()
    {
        m_shape.materialIndex = 0;
        m_shape.indexBuffer = IndexBuffer::Placeholder(0);
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
        Float3 rgb;
        float alpha;
        float scale;
    };

    struct SimpleParticle_b10
    {
        Float3 g_cameraUp;
        float padding0;
        Float3 g_cameraRight;
    };

    struct StatePerMachine
    {
        float intensity{};
        Float3 lastEmitPosition;
    };
}

struct MachineEffectDrawer::Impl : IRaceDrawer
{
    IndexBuffer m_indexBuffer{Empty};

    GenericModelDrawer m_particleDrawer{};

    StructuredBufferT<ParticleElement> m_particleBuffer{maxParticleCount};

    Array<ParticleElement> m_activeParticles{};

    ConstantBufferWrapper<SimpleParticle_b10> m_particleCB{};

    Array<StatePerMachine> m_machineStates{MaxMachineCount};

    void Init()
    {
        const auto model = std::make_shared<EffectModelBuffer>();

        m_indexBuffer = model->m_shape.indexBuffer;

        m_particleDrawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(model)
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

        m_particleCB->g_cameraUp = camera.worldMatrix().up();
        m_particleCB->g_cameraRight = camera.worldMatrix().right();
        m_particleCB.upload();

        updateParticles();

        addParticles();

        m_particleBuffer.upload(m_activeParticles);

        m_indexBuffer.resize(static_cast<int>(m_activeParticles.size()) * 6);
    }

    void drawTransparent() const override
    {
        m_particleDrawer.draw();
    }

private:
    void updateParticles()
    {
        for (int i = static_cast<int>(m_activeParticles.size()) - 1; i >= 0; --i)
        {
            m_activeParticles[i].alpha -= InGameDeltaTime();

            m_activeParticles[i].scale = Max(0.0f, m_activeParticles[i].scale - 5.0f * InGameDeltaTime());

            if (m_activeParticles[i].alpha <= 0.0f)
            {
                m_activeParticles.remove_at(i);
            }
        }
    }

    void addParticles()
    {
        auto& machineList = GetRaceContext().machineManager().machineList();
        for (int i = 0; i < machineList.size(); ++i)
        {
            const auto& machine = machineList[i];
            auto& state = m_machineStates[i];

            const bool isBoosting = machine.state.m_manualBoost > 0.0f || machine.state.m_passiveBoost > 0.0f;
            if (not isBoosting && // ブースト無し 
                state.intensity <= 0.0f) // 効果なし
            {
                continue;
            }

            if (isBoosting)
            {
                if (state.intensity == 0.0f)
                {
                    state.lastEmitPosition = machine.state.m_pose.position;
                }

                state.intensity = 1.0f;
            }
            else
            {
                state.intensity = Max(0.0f, state.intensity - InGameDeltaTime());
                if (state.intensity <= 0.0f)
                {
                    continue;
                }
            }

            const Float3 emitPosition = machine.state.m_pose.position;

            const float emitThreshold = 1.0f - 0.1f * Periodic::Sine0_1(0.1s, InGameElapsedTime());

            const float distanceSinceLastEmit = (emitPosition - state.lastEmitPosition).length();

            if (distanceSinceLastEmit >= emitThreshold)
            {
                const float emitInterval = emitThreshold;
                const int emitCount = static_cast<int>(distanceSinceLastEmit / emitInterval);
                const Float3 emitDirection = (emitPosition - state.lastEmitPosition).normalized();
                const Float3 emitRight = machine.state.rightVector();

                for (int j = 0; j < emitCount; ++j)
                {
                    if (m_activeParticles.size() >= maxParticleCount)
                    {
                        break;
                    }

                    const Float3 newParticlePos =
                        state.lastEmitPosition + emitDirection * emitInterval * (j + 1);

                    const Float3 offset = emitRight * (0.5f * Periodic::Sine1_1(0.15s, InGameElapsedTime()));

                    ParticleElement particle{};
                    particle.worldPos = newParticlePos + offset;
                    particle.rgb = machine.props.themeColor.toFloat3();
                    particle.alpha = 1.0f;
                    particle.scale = 1.0f + 1.0f * state.intensity;
                    m_activeParticles.push_back(particle);
                }

                state.lastEmitPosition = emitPosition;
            }
        }
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
