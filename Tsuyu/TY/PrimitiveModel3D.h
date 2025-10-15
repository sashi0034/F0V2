#pragma once
#include "Color.h"
#include "ModelData.h"
#include "PrimitiveTypes3D.h"

namespace TY
{
    namespace PrimitiveModel3D
    {
        ModelData Triangle(const Triangle3D& tri, const ColorF32& color);

        ModelData Quad(const Quad3D& quad, const ColorF32& color);

        ModelData Sphere(float radius, const ColorF32& color);

        ModelData Torus(float outerRadius, float innerRadius, const ColorF32& color);

        ModelData Capsule(float radius, float cylinderHeight, const ColorF32& color);

        ModelData Plane(const Float2& size, const ColorF32& color);

        ModelData TexturePlane(const TextureHandle& texture, const Float2& size = {1.0f, 1.0f});
    }
}
