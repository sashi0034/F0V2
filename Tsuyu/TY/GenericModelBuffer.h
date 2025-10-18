#pragma once
#include "TY/CbvSrvUav.h"
#include "TY/ConstantBuffer.h"
#include "TY/IndexBuffer.h"
#include "TY/VertexBuffer.h"

namespace TY
{
    struct GenericModelShapeBufferElement
    {
        uint16_t materialIndex;
        VertexBufferCore vertexBuffer{Empty};
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

        /// @remark size() = materialCount
        [[nodiscard]]
        virtual ConstantBufferImpl materialCbv() const = 0;

        /// @remark [materialCount][textureCount]
        virtual Array<Array<ShaderResourceType>> materialSrv() const = 0;
    };
}
