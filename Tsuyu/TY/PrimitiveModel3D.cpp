#include "pch.h"
#include "PrimitiveModel3D.h"

#include "Math.h"

using namespace TY;

namespace
{
    ModelShape getTextureShape(const Float2& size, int materialIndex = 0)
    {
        ModelShape shape;
        shape.materialIndex = materialIndex;

        // 頂点生成（表）
        shape.vertexBuffer = {
            {{-size.x / 2, 0, -size.y / 2}, {0, -1, 0}, {0, 1}}, // 0
            {{size.x / 2, 0, -size.y / 2}, {0, -1, 0}, {1, 1}}, // 1
            {{-size.x / 2, 0, size.y / 2}, {0, -1, 0}, {0, 0}}, // 2
            {{size.x / 2, 0, size.y / 2}, {0, -1, 0}, {1, 0}}, // 3

            // 裏面用の頂点（法線とUV反転）
            {{-size.x / 2, 0, -size.y / 2,}, {0, 1, 0}, {0, 1}}, // 4
            {{-size.x / 2, 0, size.y / 2,}, {0, 1, 0}, {0, 0}}, // 5
            {{size.x / 2, 0, -size.y / 2,}, {0, 1, 0}, {1, 1}}, // 6
            {{size.x / 2, 0, size.y / 2,}, {0, 1, 0}, {1, 0}}, // 7
        };

        // インデックス生成（表 + 裏）
        shape.indexBuffer = {
            0, 1, 2, 2, 1, 3, // 表
            4, 5, 6, 6, 5, 7 // 裏（反時計回り）
        };

        return shape;
    }
}

namespace TY
{
    ModelData PrimitiveModel3D::Triangle(const Triangle3D& tri, const ColorF32& color)
    {
        ModelData data;

        ModelMaterialParameters params;
        params.ambient = color.toFloat3() * 0.1f; // Ambient は拡散成分の 10%
        params.diffuse = color.toFloat3();
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"Triangle", params, {}});

        ModelShape shape;
        shape.materialIndex = 0; // 上で追加したマテリアルを参照

        // 頂点生成
        Float3 norm = tri.getNormal();
        shape.vertexBuffer = {
            {tri.p0, norm, {0, 0}},
            {tri.p1, norm, {1, 0}},
            {tri.p2, norm, {0, 1}},

            // 裏面用の頂点（法線とUV反転）
            {tri.p0, -norm, {0, 0}},
            {tri.p2, -norm, {0, 1}},
            {tri.p1, -norm, {1, 0}},
        };

        // インデックス生成（表 + 裏）
        shape.indexBuffer = {
            0, 1, 2, // 表
            3, 4, 5 // 裏（反時計回り）
        };

        data.shapes.push_back(std::move(shape));
        return data;
    }

    ModelData PrimitiveModel3D::Quad(const Quad3D& quad, const ColorF32& color)
    {
        ModelData data;

        ModelMaterialParameters params;
        params.ambient = color.toFloat3() * 0.1f; // Ambient は拡散成分の 10%
        params.diffuse = color.toFloat3();
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"Quad", params, {}});

        ModelShape shape;
        shape.materialIndex = 0; // 上で追加したマテリアルを参照

        // 頂点生成
        Float3 norm = quad.getNormal();
        shape.vertexBuffer = {
            {quad.p0, norm, {0, 0}},
            {quad.p1, norm, {1, 0}},
            {quad.p2, norm, {0, 1}},
            {quad.p3, norm, {1, 1}},

            // 裏面用の頂点（法線とUV反転）
            {quad.p0, -norm, {0, 0}},
            {quad.p2, -norm, {0, 1}},
            {quad.p1, -norm, {1, 0}},
            {quad.p3, -norm, {1, 1}},
        };

        // インデックス生成（表 + 裏）
        shape.indexBuffer = {
            0, 1, 2, 2, 1, 3, // 表
            4, 5, 6, 6, 5, 7 // 裏（反時計回り）
        };

        data.shapes.push_back(std::move(shape));
        return data;
    }

    ModelData PrimitiveModel3D::Sphere(float radius, const ColorF32& color)
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
                shape.indexBuffer.push_back(i1);
                shape.indexBuffer.push_back(i2);

                // 下三角形
                shape.indexBuffer.push_back(i1);
                shape.indexBuffer.push_back(i3);
                shape.indexBuffer.push_back(i2);
            }
        }

        data.shapes.push_back(std::move(shape));
        return data;
    }

    ModelData PrimitiveModel3D::Capsule(float radius, float cylinderHeight, const ColorF32& color)
    {
        // 解像度（必要に応じて調整）
        constexpr uint16_t sliceCount = 32; // 経線方向
        constexpr uint16_t hemiStacks = 8; // 半球の緯線数（片側）
        constexpr uint16_t cylStacks = 8; // シリンダ緯線数

        const float halfCyl = cylinderHeight * 0.5f;
        constexpr uint32_t rings = static_cast<uint32_t>(hemiStacks + cylStacks + hemiStacks) + 1; // 垂直リング総数

        ModelData data;

        // マテリアル（スフィアと同様）
        ModelMaterialParameters params;
        params.ambient = color.toFloat3() * 0.1f;
        params.diffuse = color.toFloat3();
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"Capsule", params, {}});

        ModelShape shape;
        shape.materialIndex = 0;

        shape.vertexBuffer.reserve(size_t(rings) * size_t(sliceCount + 1));

        const float totalHeight = cylinderHeight + 2.0f * radius; // UV用
        const float yTop = halfCyl + radius; // 最高点Y
        const float yBottom = -halfCyl - radius; // 最低点Y

        auto safe_normalize = [](Float3 v) -> Float3
        {
            float len2 = v.x * v.x + v.y * v.y + v.z * v.z;
            if (len2 > 0.0f)
            {
                float inv = 1.0f / std::sqrt(len2);
                return {v.x * inv, v.y * inv, v.z * inv};
            }
            return {0, 1, 0};
        };

        // 垂直リングごとに頂点生成（上 --> 下）
        for (uint32_t i = 0; i < rings; ++i)
        {
            float y = 0.0f;
            float ringR = radius; // そのリングの半径（XZ平面）
            enum class Region { TopHemi, Cylinder, BottomHemi } region;

            if (i <= hemiStacks)
            {
                // 上半球: φ ∈ [0, π/2]
                float t = float(i) / float(hemiStacks);
                float phi = t * (Math::PiF * 0.5f);
                y = halfCyl + radius * std::cos(phi);
                ringR = radius * std::sin(phi);
                region = Region::TopHemi;
            }
            else if (i <= hemiStacks + cylStacks)
            {
                // シリンダ: y ∈ [ +halfCyl, -halfCyl ]
                float t = float(i - hemiStacks) / float(cylStacks);
                y = (1.0f - t) * (halfCyl) + t * (-halfCyl);
                ringR = radius;
                region = Region::Cylinder;
            }
            else
            {
                // 下半球: φ ∈ [0, π/2]　（赤道 --> 下極）
                float t = float(i - (hemiStacks + cylStacks)) / float(hemiStacks);
                float phi = t * (Math::PiF * 0.5f);
                y = -halfCyl - radius * std::sin(phi);
                ringR = radius * std::cos(phi);
                region = Region::BottomHemi;
            }

            // V（縦UV）: 上端0 --> 下端1
            float v = (yTop - y) / totalHeight;

            for (uint32_t j = 0; j <= sliceCount; ++j)
            {
                float u = float(j) / float(sliceCount);
                float theta = 2.0f * Math::PiF * u;

                float x = ringR * std::cos(theta);
                float z = ringR * std::sin(theta);

                Float3 pos{x, y, z};

                // 法線：半球は中心（±halfCyl）からの方向、シリンダは水平
                Float3 n;
                if (region == Region::TopHemi)
                {
                    n = safe_normalize({x, y - halfCyl, z});
                }
                else if (region == Region::Cylinder)
                {
                    n = safe_normalize({x, 0.0f, z});
                }
                else // BottomHemi
                {
                    n = safe_normalize({x, y + halfCyl, z});
                }

                Float2 uv{u, v};

                shape.vertexBuffer.push_back({pos, n, uv});
            }
        }

        // インデックス生成（各リング間の四角を三角2枚に）
        constexpr uint32_t ringStride = sliceCount + 1;
        for (uint32_t i = 0; i < rings - 1; ++i)
        {
            for (uint32_t j = 0; j < sliceCount; ++j)
            {
                uint32_t a = i * ringStride + j;
                uint32_t b = (i + 1) * ringStride + j;

                // CCW
                shape.indexBuffer.push_back(a);
                shape.indexBuffer.push_back(a + 1);
                shape.indexBuffer.push_back(b);

                shape.indexBuffer.push_back(a + 1);
                shape.indexBuffer.push_back(b + 1);
                shape.indexBuffer.push_back(b);
            }
        }

        data.shapes.push_back(std::move(shape));
        return data;
    }

    ModelData PrimitiveModel3D::Plane(const Float2& size, const ColorF32& color)
    {
        ModelData data;

        ModelMaterialParameters params;
        params.ambient = {0.1f, 0.1f, 0.1f};
        params.diffuse = color.toFloat3();
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"Plane", params, {}});

        data.shapes.push_back(getTextureShape(size));
        return data;
    }

    ModelData PrimitiveModel3D::TexturePlane(const TextureResource& texture, const Float2& size)
    {
        ModelData data;

        ModelMaterialParameters params;
        params.ambient = {0.1f, 0.1f, 0.1f};
        params.diffuse = {1.0f, 1.0f, 1.0f};
        params.specular = {1.0f, 1.0f, 1.0f};
        params.shininess = 32.0f;

        data.materials.push_back({"TexturePlane", params, texture});

        data.shapes.push_back(getTextureShape(size));
        return data;
    }
}
