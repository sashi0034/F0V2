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
            ConstantBufferImpl materialCbv = ConstantBufferImpl{Empty},
            Array<ShaderResourceType> materialSrv = {})
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = std::move(indexBuffer);
            m_materialCbv.push_back({std::move(materialCbv)});

            if (not materialSrv.empty())
            {
                m_materialSrv.push_back(std::move(materialSrv));
            }
        }

        explicit SingleShapeModelBuffer(
            int placeholderIndexCount,
            ConstantBufferImpl materialCbv = ConstantBufferImpl{Empty},
            Array<ShaderResourceType> materialSrv = {})
        {
            assert(placeholderIndexCount >= 0);

            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(placeholderIndexCount);
            m_materialCbv.push_back({std::move(materialCbv)});

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

        const Array<Array<ConstantBufferImpl>>& materialCbv() const override
        {
            return m_materialCbv;
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return m_materialSrv;
        }

    private:
        GenericModelShapeBufferElement m_shape{};

        Array<Array<ConstantBufferImpl>> m_materialCbv{};

        Array<Array<ShaderResourceType>> m_materialSrv{};
    };
}
