#pragma once
#include "MaterialList.h"
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBuffer.h"
#include "TY/IndexBuffer.h"
#include "TY/VertexBuffer.h"

namespace TY
{
    struct GenericModelShapeBufferElement
    {
        uint16_t materialIndex;
        VertexBufferImpl vertexBuffer{Empty};
        IndexBuffer indexBuffer{Empty};
    };

    class IGenericModelBuffer
    {
    public:
        virtual ~IGenericModelBuffer() = default;

        [[nodiscard]]
        virtual int shapeCount() const = 0;

        [[nodiscard]]
        virtual GenericModelShapeBufferElement shapeAt(int index) const = 0;

        [[nodiscard]]
        virtual int materialCount() const = 0;

        /// @remark [materialCount][1]
        [[nodiscard]]
        virtual const MaterialList<DescriptorList<ConstantBufferImpl>>& materialCbv() const = 0;

        /// @remark [materialCount][textureCount]
        virtual MaterialList<DescriptorList<ShaderResourceType>> materialSrv() const = 0;
    };
}
