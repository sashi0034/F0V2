#pragma once
#include "ConstantBufferUploader.h"
#include "IndexBuffer.h"
#include "ModelData.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"

namespace TY
{
    struct ModelShapeBufferElement
    {
        uint16_t materialIndex;
        VertexBuffer<ModelVertex> vertexBuffer;
        IndexBuffer indexBuffer;
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

        size_t materialCount() const;

        const ConstantBufferUploader<ModelMaterialParameters>& materialCB() const;

        /// @brief [textureCount][materialCount]
        const Array<Array<ShaderResourceType>>& materialTextures() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
