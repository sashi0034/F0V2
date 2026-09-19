#pragma once
#include "Array.h"
#include "TextureHandle.h"
#include "Vector2D.h"
#include "Vector3D.h"

namespace TY
{
    struct ModelVertex
    {
        Float3 position{};
        Float3 normal{};
        Float2 texCoord{};
    };

    struct ModelShape
    {
        Array<ModelVertex> vertexBuffer{};
        Array<uint16_t> indexBuffer{};
        uint16_t materialIndex{};
    };

    struct ModelMaterialParameters
    {
        alignas(16) Float3 albedo{};

        // TODO: 必要なものを追加 (場合によっては union も使用)
    };

    struct ModelMaterial
    {
        std::string name{};
        ModelMaterialParameters parameters{};
        TextureHandle albedoTexture{};
    };

    struct ModelData
    {
        Array<ModelShape> shapes{};
        Array<ModelMaterial> materials{};

        ModelShape& takeShapeByMaterialIndex(uint16_t materialIndex);
    };
}
