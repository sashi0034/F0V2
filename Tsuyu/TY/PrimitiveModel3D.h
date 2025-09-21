#pragma once
#include "ModelData.h"
#include "Triangle3D.h"

namespace TY
{
    namespace PrimitiveModel3D
    {
        ModelData Triangle(const Triangle3D& tri, const ColorF32& color);

        ModelData Sphere(float radius, const ColorF32& color);

        ModelData Plane(const Float2& size, const ColorF32& color);

        ModelData TexturePlane(const TextureResource& texture, const Float2& size = {1.0f, 1.0f});
    }
}
