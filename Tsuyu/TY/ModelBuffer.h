#pragma once
#include "GenericModelBuffer.h"
#include "IndexBuffer.h"
#include "ModelData.h"
#include "VertexBuffer.h"

namespace TY
{
    struct ModelShapeBufferElement
    {
        uint16_t materialIndex;
        VertexBuffer<ModelVertex> vertexBuffer{Empty};
        IndexBuffer indexBuffer{Empty};

        GenericModelShapeBufferElement asGeneric() const;
    };

    class ModelShapeBuffer
    {
    public:
        ModelShapeBuffer() = default;

        ModelShapeBuffer(const Array<ModelShape>& shapes);

        const Array<ModelShapeBufferElement>& shapes() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    class ModelBuffer
    {
    public:
        ModelBuffer() = default;

        ModelBuffer(const ModelData& modelData);

        ModelBuffer(const ModelShapeBuffer& shapes, const Array<ModelMaterial>& materials);

        const ModelShapeBuffer& shapeBuffer() const;

        const ConstantBuffer<ModelMaterialParameters>& materialCbv() const;

        std::shared_ptr<IGenericModelBuffer> asGeneric() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
