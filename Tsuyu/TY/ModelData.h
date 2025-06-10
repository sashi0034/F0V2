#pragma once
#include "Array.h"
#include "ShaderResourceTexture.h"
#include "Value2D.h"
#include "Value3D.h"

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
        alignas(16) Float3 ambient{};
        alignas(16) Float3 diffuse{};
        alignas(16) Float3 specular{};
        alignas(16) float shininess{};
    };

    struct ModelMaterial
    {
        std::string name{};
        ModelMaterialParameters parameters{};
        ShaderResourceTexture diffuseTexture{};
    };

    struct ModelData
    {
        Array<ModelShape> shapes{};
        Array<ModelMaterial> materials{};
    };
}
