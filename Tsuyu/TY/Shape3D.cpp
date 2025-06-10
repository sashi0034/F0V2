#include "pch.h"
#include "Shape3D.h"

#include "Math.h"

namespace TY
{
    ModelData Shape3D::Sphere(float radius, const ColorF32& color)
    {
        // 解像度
        constexpr uint16_t sliceCount = 32;
        constexpr uint16_t stackCount = 16;

        ModelData data;

        ModelMaterialParameters params;
        params.ambient = color.toFloat3() * 0.1f; // Ambient は拡散成分の 10%
        params.diffuse = color.toFloat3();
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"Sphere", params, {}});

        ModelShape shape;
        shape.materialIndex = 0; // 上で追加したマテリアルを参照

        // 頂点生成
        for (unsigned int i = 0; i <= stackCount; ++i)
        {
            float phi = Math::PiF * float(i) / float(stackCount); // 0, π
            float y = radius * std::cos(phi);
            float r = radius * std::sin(phi);

            for (unsigned int j = 0; j <= sliceCount; ++j)
            {
                float theta = 2.0f * Math::PiF * float(j) / float(sliceCount); // 0, 2π
                float x = r * std::cos(theta);
                float z = r * std::sin(theta);

                Float3 pos = {x, y, z};
                Float3 norm = {x / radius, y / radius, z / radius};
                Float2 uv = {float(j) / float(sliceCount), float(i) / float(stackCount)};

                shape.vertexBuffer.push_back({pos, norm, uv});
            }
        }

        // インデックス生成
        // 各スタックでの四角形を ２ 三角形に分割
        const unsigned int ringVertexCount = sliceCount + 1;
        for (unsigned int i = 0; i < stackCount; ++i)
        {
            for (unsigned int j = 0; j < sliceCount; ++j)
            {
                uint16_t i0 = uint16_t(i * ringVertexCount + j);
                uint16_t i1 = uint16_t(i * ringVertexCount + j + 1);
                uint16_t i2 = uint16_t((i + 1) * ringVertexCount + j);
                uint16_t i3 = uint16_t((i + 1) * ringVertexCount + j + 1);

                // 上三角形
                shape.indexBuffer.push_back(i0);
                shape.indexBuffer.push_back(i2);
                shape.indexBuffer.push_back(i1);

                // 下三角形
                shape.indexBuffer.push_back(i1);
                shape.indexBuffer.push_back(i2);
                shape.indexBuffer.push_back(i3);
            }
        }

        data.shapes.push_back(std::move(shape));
        return data;
    }
}
