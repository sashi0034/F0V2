#include "pch.h"
#include "ModelData.h"

namespace TY
{
    ModelShape& ModelData::takeShapeByMaterialIndex(uint16_t materialIndex)
    {
        for (auto& shape : shapes)
        {
            if (shape.materialIndex == materialIndex)
            {
                return shape;
            }
        }

        shapes.emplace_back();
        auto& newShape = shapes.back();
        newShape.materialIndex = materialIndex;
        return newShape;
    }
}
