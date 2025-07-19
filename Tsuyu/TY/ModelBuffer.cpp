#include "pch.h"
#include "ModelBuffer.h"

#include "ConstantBufferUploader.h"
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

struct ModelBuffer::Impl
{
    ModelShapeBuffer m_shapeBuffer{};

    ConstantBufferUploader<ModelMaterialParameters> m_materialCB{Empty};

    Array<Array<ShaderResourceType>> m_materialTextures{};

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
        m_materialCB = ConstantBufferUploader<ModelMaterialParameters>{
            materials.map([](const ModelMaterial& material)
            {
                return material.parameters;
            })
        };

        m_materialTextures.push_back(materials.map([](const ModelMaterial& material)
        {
            return ShaderResourceType(material.diffuseTexture);
        }));

        // TODO: Add another texture types if needed
    }
};

namespace TY
{
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
    {
    }

    const ModelShapeBuffer& ModelBuffer::shapeBuffer() const
    {
        return p_impl->m_shapeBuffer;
    }

    size_t ModelBuffer::materialCount() const
    {
        return p_impl->m_materialCB.materialCount();
    }

    const ConstantBufferUploader<ModelMaterialParameters>& ModelBuffer::materialCB() const
    {
        return p_impl->m_materialCB;
    }

    const Array<Array<ShaderResourceType>>& ModelBuffer::materialTextures() const
    {
        return p_impl->m_materialTextures;
    }
}
