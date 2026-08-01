#pragma once
#include <cassert>
#include <utility>

#include "GenericModelBuffer.h"

namespace TY
{
    class SingleShapeModelBuffer final : public IGenericModelBuffer
    {
    public:
        explicit SingleShapeModelBuffer(
            IndexBuffer indexBuffer,
            ConstantBufferArrayImpl materialCbv = ConstantBufferArrayImpl{Empty},
            Array<ShaderResourceType> materialSrv = {})
            : m_materialCbv(std::move(materialCbv))
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = std::move(indexBuffer);

            if (not materialSrv.empty())
            {
                m_materialSrv.push_back(std::move(materialSrv));
            }
        }

        explicit SingleShapeModelBuffer(
            int placeholderIndexCount,
            ConstantBufferArrayImpl materialCbv = ConstantBufferArrayImpl{Empty},
            Array<ShaderResourceType> materialSrv = {})
            : m_materialCbv(std::move(materialCbv))
        {
            assert(placeholderIndexCount >= 0);

            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(placeholderIndexCount);

            if (not materialSrv.empty())
            {
                m_materialSrv.push_back(std::move(materialSrv));
            }
        }

        int shapeCount() const override
        {
            return 1;
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            assert(index == 0);
            return m_shape;
        }

        int materialCount() const override
        {
            return 1;
        }

        ConstantBufferArrayImpl materialCbv() const override
        {
            return m_materialCbv;
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return m_materialSrv;
        }

    private:
        GenericModelShapeBufferElement m_shape{};

        ConstantBufferArrayImpl m_materialCbv{Empty};

        Array<Array<ShaderResourceType>> m_materialSrv{};
    };
}
