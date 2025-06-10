#include "pch.h"
#include "ModelLoader.h"

#include "Logger.h"

using namespace TY;

namespace
{
    using ShapeData = ModelShape;

    struct IndexKey
    {
        uint16_t v;
        uint16_t t;
        uint16_t n;
        uint16_t padding;

        bool operator==(const IndexKey& other) const
        {
            return (v == other.v) && (t == other.t) && (n == other.n);
        }
    };

    struct IndexKeyHash
    {
        size_t operator()(const IndexKey& key) const
        {
            static_assert(sizeof(size_t) == sizeof(IndexKey));
            return *reinterpret_cast<const size_t*>(&key);
        }
    };

    ModelShape& takeShapeByMaterialIndex(ModelData& modelData, uint16_t materialIndex)
    {
        for (auto& shape : modelData.shapes)
        {
            if (shape.materialIndex == materialIndex)
            {
                return shape;
            }
        }

        modelData.shapes.emplace_back();
        auto& newShape = modelData.shapes.back();
        newShape.materialIndex = materialIndex;
        return newShape;
    }

    ModelData loadWavefront(const std::string& filename)
    {
        tinyobj::attrib_t attrib{};
        Array<tinyobj::shape_t> shapes{};
        Array<tinyobj::material_t> materials{};

        const auto baseDir = std::filesystem::path(filename).parent_path().string();
        if (not tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filename.c_str(), baseDir.c_str()))
        {
            LogError.writeln("Failed to load obj file: " + filename);
            return {};
        }

        // 頂点座標の取得
        Array<Float3> positions{};
        for (size_t i = 0; i < attrib.vertices.size(); i += 3)
        {
            positions.emplace_back(attrib.vertices[i], attrib.vertices[i + 1], attrib.vertices[i + 2]);
        }

        // UV座標の取得
        Array<Float2> texCoords{};
        for (size_t i = 0; i < attrib.texcoords.size(); i += 2)
        {
            texCoords.emplace_back(attrib.texcoords[i], 1.0f - attrib.texcoords[i + 1]);
        }

        // 法線の取得
        Array<Float3> normals{};
        for (size_t i = 0; i < attrib.normals.size(); i += 3)
        {
            normals.emplace_back(attrib.normals[i], attrib.normals[i + 1], attrib.normals[i + 2]);
        }

        // 面データの取得
        ModelData modelData{};
        for (const auto& shape : shapes)
        {
            if (shape.mesh.num_face_vertices.empty()) continue;;

            std::unordered_map<IndexKey, unsigned int, IndexKeyHash> indexMap{};

            ShapeData* shapeData{nullptr};
            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
            {
                if (not shapeData || shapeData->materialIndex != shape.mesh.material_ids[f])
                {
                    shapeData = &takeShapeByMaterialIndex(modelData, shape.mesh.material_ids[f]);
                    assert(shapeData->materialIndex == shape.mesh.material_ids[f]);
                }

                const int fv = shape.mesh.num_face_vertices[f]; // 1 つの面の頂点数（三角形なら 3）
                for (int v = 0; v < fv; ++v)
                {
                    const auto& index = shape.mesh.indices[indexOffset + v];
                    IndexKey key{};
                    key.v = index.vertex_index; // TODO: uint16 オーバーフロー対策?
                    key.t = index.texcoord_index;
                    key.n = index.normal_index;

                    const auto iter = indexMap.find(key);
                    if (iter == indexMap.end())
                    {
                        ModelVertex vertex{};
                        vertex.position = positions[index.vertex_index];
                        if (index.texcoord_index >= 0)
                        {
                            vertex.texCoord = texCoords[index.texcoord_index];
                        }
                        if (index.normal_index >= 0)
                        {
                            vertex.normal = normals[index.normal_index];
                        }

                        const auto newIndex = shapeData->vertexBuffer.size();
                        shapeData->vertexBuffer.push_back(vertex);
                        indexMap[key] = newIndex;
                        shapeData->indexBuffer.push_back(newIndex);
                    }
                    else
                    {
                        shapeData->indexBuffer.push_back(iter->second);
                    }
                }

                indexOffset += fv;
            }
        }

        // Material 情報の変換
        for (const auto& m : materials)
        {
            ModelMaterial material{};

            material.name = m.name;

            material.diffuseTexture =
                m.diffuse_texname.empty()
                    ? ShaderResourceTexture{}
                    : ShaderResourceTexture{baseDir + "/" + m.diffuse_texname};

            material.parameters.ambient = Float3(m.ambient[0], m.ambient[1], m.ambient[2]);
            material.parameters.diffuse = Float3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
            material.parameters.specular = Float3(m.specular[0], m.specular[1], m.specular[2]);
            material.parameters.shininess = m.shininess;

            modelData.materials.push_back(material);
        }

        return modelData;
    }
}

namespace TY
{
    ModelData ModelLoader::FromWavefront(const std::string& filename)
    {
        return loadWavefront(filename);
    }
}
