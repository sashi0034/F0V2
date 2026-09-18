#pragma once
#include <cstddef>
#include <cstdint>

#include "CouseFaceType.h"
#include "TY/Array.h"
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
        uint32_t textureIndex{}; // offset 36 : TEXCOORD2  R32_UINT (CourseTextureKind)
        float metadata{}; // offset 40 : TEXCOORD3  R32_FLOAT
    };

    static_assert(sizeof(CourseModelVertex) == 44);
    static_assert(offsetof(CourseModelVertex, uv) == 24);
    static_assert(offsetof(CourseModelVertex, faceType) == 32);
    static_assert(offsetof(CourseModelVertex, textureIndex) == 36);
    static_assert(offsetof(CourseModelVertex, metadata) == 40);

    struct CourseModelShape
    {
        Array<CourseModelVertex> vertexBuffer{};
        Array<uint16_t> indexBuffer{};
    };

    struct CourseModelData
    {
        Array<CourseModelShape> shapes{};

        // NOTE: テクスチャはコース共通のものを頂点の textureIndex で引く (つまりマテリアル共通)
        // NOTE: 色情報などはシェーダー内でハードコードしている
    };
}
