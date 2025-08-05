#pragma once
#include "ModelData.h"

namespace TY
{
    namespace PrimitiveModel3D
    {
        ModelData Sphere(float radius, const ColorF32& color);

        ModelData Plane(const Float2& size, const ColorF32& color);

        ModelData TexturePlane(const ShaderResourceTexture& texture, const Float2& size = {1.0f, 1.0f});
    }
}
