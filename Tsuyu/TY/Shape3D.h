#pragma once
#include "ModelData.h"

namespace TY
{
    namespace Shape3D
    {
        ModelData Sphere(float radius, const ColorF32& color);

        ModelData TexturePlane(const ShaderResourceTexture& texture, const Float2& size = {1.0f, 1.0f});
    }
}
