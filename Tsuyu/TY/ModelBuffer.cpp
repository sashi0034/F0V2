#include "pch.h"
#include "ModelBuffer.h"

#include "ConstantBufferArray.h"
#include "IndexBuffer.h"
#include "VertexBuffer.h"

using namespace TY;
using namespace TY::detail;

struct ModelShapeBuffer::Impl
{
    Array<ModelShapeBufferElement> m_shapes{};

    Impl(const Array<ModelShape>& shapes)
    {
        for (auto& shape : shapes)
        {
            ModelShapeBufferElement shapeBuffer{};
            shapeBuffer.materialIndex = shape.materialIndex;
            shapeBuffer.vertexBuffer = VertexBuffer(shape.vertexBuffer);
            shapeBuffer.indexBuffer = IndexBuffer{shape.indexBuffer};
            m_shapes.push_back(shapeBuffer);
        }
    }
};

struct ModelBuffer::Impl : IGenericModelBuffer
{
    ModelShapeBuffer m_shapeBuffer{};

    ConstantBufferArray<ModelMaterialParameters> m_materialCbv{Empty};

    Array<Array<ShaderResourceType>> m_materialSrv{};

    Impl(const ModelData& modelData)
        : m_shapeBuffer(modelData.shapes)
    {
        initializeMaterial(modelData.materials);
    }

    Impl(const ModelShapeBuffer& shapes, const Array<ModelMaterial>& materials)
        : m_shapeBuffer(shapes)
    {
        initializeMaterial(materials);
    }

    void initializeMaterial(const Array<ModelMaterial>& materials)
    {
        m_materialCbv = ConstantBufferArray<ModelMaterialParameters>{
            materials.map([](const ModelMaterial& material)
            {
                return material.parameters;
            })
        };

        for (const ModelMaterial& material : materials)
        {
            m_materialSrv.push_back({material.diffuseTexture});
        }

        // TODO: Add another texture types if needed
    }

    [[nodiscard]] int shapeCount() const override
    {
        return static_cast<int>(m_shapeBuffer.shapes().size());
    }

    [[nodiscard]] GenericModelShapeBufferElement shapeAt(int index) const override
    {
        return m_shapeBuffer.shapes()[index].asGeneric();
    }

    [[nodiscard]] int materialCount() const override
    {
        return static_cast<int>(m_materialCbv.materialCount());
    }

    [[nodiscard]] ConstantBufferArrayImpl materialCbv() const override
    {
        return m_materialCbv;
    }

    Array<Array<ShaderResourceType>> materialSrv() const override
    {
        return m_materialSrv;
    }
};

namespace TY
{
    GenericModelShapeBufferElement ModelShapeBufferElement::asGeneric() const
    {
        GenericModelShapeBufferElement element;
        element.indexBuffer = indexBuffer;
        element.vertexBuffer = vertexBuffer;
        element.materialIndex = materialIndex;
        return element;
    }

    ModelShapeBuffer::ModelShapeBuffer(const Array<ModelShape>& shapes)
        : p_impl(std::make_shared<Impl>(shapes))
    {
    }

    const Array<ModelShapeBufferElement>& ModelShapeBuffer::shapes() const
    {
        return p_impl->m_shapes;
    }

    ModelBuffer::ModelBuffer(const ModelData& modelData)
        : p_impl(std::make_shared<Impl>(modelData))
    {
    }

    ModelBuffer::ModelBuffer(const ModelShapeBuffer& shapes, const Array<ModelMaterial>& materials)
        : p_impl(std::make_shared<Impl>(shapes, materials))
    {
    }

    const ModelShapeBuffer& ModelBuffer::shapeBuffer() const
    {
        return p_impl->m_shapeBuffer;
    }

    const ConstantBufferArray<ModelMaterialParameters>& ModelBuffer::materialCbv() const
    {
        return p_impl->m_materialCbv;
    }

    std::shared_ptr<IGenericModelBuffer> ModelBuffer::asGeneric() const
    {
        return p_impl;
    }
}
