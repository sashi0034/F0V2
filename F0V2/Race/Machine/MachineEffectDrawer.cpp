#include "pch.h"
#include "MachineEffectDrawer.h"

#include "Asset.generated.h"
#include "TY/Array.h"
#include "TY/ConstantBufferArray.h"
#include "TY/GenericModelBuffer.h"
#include "TY/GenericModelDrawer.h"

using namespace Race;

struct EffectModelBuffer : IGenericModelBuffer
{
    GenericModelShapeBufferElement m_shape{};

    EffectModelBuffer()
    {
        m_shape.materialIndex = 0;
        m_shape.indexBuffer = IndexBuffer::Placeholder(6);
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

struct MachineEffectDrawer::Impl
{
    GenericModelDrawer m_fireDrawer{};

    void Init()
    {
        m_fireDrawer = GenericModelDrawer{
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
            .setSrv10AndLater({Asset_image::particle.fetchResource()})
        };
    }

    void Update()
    {
    }

    void Draw() const
    {
        m_fireDrawer.draw();
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
    }

    void MachineEffectDrawer::update()
    {
        p_impl->Update();
    }

    void MachineEffectDrawer::drawTransparent() const
    {
        p_impl->Draw();
    }
}
