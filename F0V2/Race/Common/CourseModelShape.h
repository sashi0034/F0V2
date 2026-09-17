#pragma once
#include <cstddef>
#include <cstdint>

#include "CouseFaceType.h"
#include "TY/Array.h"
#include "TY/TextureHandle.h"
#include "TY/Vector2D.h"
#include "TY/Vector3D.h"

namespace Race
{
    /// @brief gbuffer_course*.hlsl の頂点入力
    struct CourseModelVertex
    {
        Float3 position{}; // offset  0 : POSITION0  R32G32B32_FLOAT
        Float3 normal{}; // offset 12 : NORMAL0    R32G32B32_FLOAT
        Float2 uv{}; // offset 24 : TEXCOORD0  R32G32_FLOAT
        uint32_t faceType{}; // offset 32 : TEXCOORD1  R32_UINT (CourseFaceType)
        float metadata{}; // offset 36 : TEXCOORD2  R32_FLOAT
    };

    static_assert(sizeof(CourseModelVertex) == 40);
    static_assert(offsetof(CourseModelVertex, uv) == 24);
    static_assert(offsetof(CourseModelVertex, faceType) == 32);
    static_assert(offsetof(CourseModelVertex, metadata) == 36);

    struct CourseModelShape
    {
        Array<CourseModelVertex> vertexBuffer{};
        Array<uint16_t> indexBuffer{};
        uint16_t materialIndex{};
    };

    struct CourseModelMaterial
    {
        std::string name{};
        TextureHandle texture{};

        // NOTE: 色情報などはシェーダー内でハードコードしている
    };

    struct CourseModelData
    {
        Array<CourseModelShape> shapes{};
        Array<CourseModelMaterial> materials{};

        /// @brief 同名のマテリアルがあれば再利用し、なければ追加する
        uint16_t takeMaterialIndex(const std::string_view name, const TextureHandle& texture = TextureHandle{})
        {
            for (int i = 0; i < materials.size(); ++i)
            {
                if (materials[i].name == name) return static_cast<uint16_t>(i);
            }

            materials.push_back(CourseModelMaterial{std::string{name}, texture});
            return static_cast<uint16_t>(materials.size() - 1);
        }
    };
}
