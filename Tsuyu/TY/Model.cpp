#include "pch.h"
#include "Model.h"

#include "Array.h"
#include "AssertObject.h"
#include "ConstantBuffer.h"
#include "Graphics3D.h"
#include "IndexBuffer.h"
#include "Mat4x4.h"
#include "Utils.h"
#include "Value2D.h"
#include "Value3D.h"
#include "VertexBuffer.h"
#include "detail/DescriptorHeap.h"
#include "detail/EngineCore.h"
#include "detail/EnginePresetAsset.h"
#include "detail/EngineStackState.h"
#include "detail/PipelineState.h"

using namespace TY;

namespace
{
    struct ModelVertex
    {
        Float3 position;
        Float3 normal;
        Float2 texCoord;
    };

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

    struct IndexKeyHasher
    {
        size_t operator()(const IndexKey& key) const
        {
            static_assert(sizeof(size_t) == sizeof(IndexKey));
            return *reinterpret_cast<const size_t*>(&key);
        }
    };

    struct ModelMaterial_b
    {
        alignas(16) Float3 ambient; // アンビエント色
        alignas(16) Float3 diffuse; // 拡散反射色
        alignas(16) Float3 specular; // 鏡面反射色
        alignas(16) float shininess; // 光沢
    };

    struct ShapeData
    {
        Array<ModelVertex> vertexBuffer{};
        Array<uint16_t> indexBuffer{};
        uint16_t materialIndex{};
    };

    struct ModelData
    {
        Array<ShapeData> shapes{};
        Array<std::string> materialNames{};
        Array<std::string> materialDiffuseTextures{};
        Array<ModelMaterial_b> materials{};
    };

    ModelData loadObj(const std::string& filename)
    {
        tinyobj::attrib_t attrib{};
        Array<tinyobj::shape_t> shapes{};
        Array<tinyobj::material_t> materials{};

        const auto baseDir = std::filesystem::path(filename).parent_path().string();
        if (not tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, filename.c_str(), baseDir.c_str()))
        {
            // TODO: handle error
            throw std::runtime_error("failed to load obj file");
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
            ShapeData shapeData{};
            shapeData.materialIndex = shape.mesh.material_ids.empty() ? 0 : shape.mesh.material_ids[0];

            std::unordered_map<IndexKey, unsigned int, IndexKeyHasher> indexMap{};

            size_t indexOffset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
            {
                const int fv = shape.mesh.num_face_vertices[f]; // 1つの面の頂点数（三角形なら3）

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

                        const auto newIndex = shapeData.vertexBuffer.size();
                        shapeData.vertexBuffer.push_back(vertex);
                        indexMap[key] = newIndex;
                        shapeData.indexBuffer.push_back(newIndex);
                    }
                    else
                    {
                        shapeData.indexBuffer.push_back(iter->second);
                    }
                }

                indexOffset += fv;
            }

            modelData.shapes.push_back(shapeData);
        }

        // Material 情報の変換
        for (const auto& m : materials)
        {
            modelData.materialNames.push_back(m.name);

            modelData.materialDiffuseTextures.push_back(m.diffuse_texname.empty()
                                                            ? ""
                                                            : baseDir + "/" + m.diffuse_texname);

            ModelMaterial_b modelMat;
            modelMat.ambient = Float3(m.ambient[0], m.ambient[1], m.ambient[2]);
            modelMat.diffuse = Float3(m.diffuse[0], m.diffuse[1], m.diffuse[2]);
            modelMat.specular = Float3(m.specular[0], m.specular[1], m.specular[2]);
            modelMat.shininess = m.shininess;
            modelData.materials.push_back(modelMat);
        }

        return modelData;
    }

    using namespace TY;
    using namespace TY::detail;

    const DescriptorTable baseDescriptorTable = {{1, 0, 0}, {1, 1, 0}};

    PipelineState makePipelineState(const ModelParams& params)
    {
        auto descriptorTable = baseDescriptorTable;
        if (params.cb2.has_value())
        {
            descriptorTable.push_back({1, 0, 0});;
        }

        // TODO: キャッシュする?
        return PipelineState{
            PipelineStateParams{
                .pixelShader = params.ps,
                .vertexShader = params.vs,
                .vertexInput = {
                    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT}
                },
                .hasDepth = true,
                .descriptorTable = descriptorTable,
            }
        };
    }

    struct SceneState_b0
    {
        Mat4x4 worldMat;
        Mat4x4 viewMat;
        Mat4x4 projectionMat;
    };

    struct ShapeBuffer
    {
        VertexBuffer<ModelVertex> vertexBuffer;
        IndexBuffer indexBuffer;
    };
}

struct Model::Impl
{
    ModelData m_modelData{};
    Array<Array<ShapeBuffer>> m_shapes{};
    PipelineState m_pipelineState;

    DescriptorHeap m_descriptorHeap{};

    ConstantBuffer<SceneState_b0> m_cb0{};

    ConstantBuffer<ModelMaterial_b> m_cb1{};

    std::optional<ConstantBuffer_impl> m_cb2{};

    Impl(const ModelParams& params) :
        m_modelData(loadObj(params.filename)),
        m_pipelineState(makePipelineState(params)),
        m_cb2(params.cb2)
    {
        m_shapes.resize(m_modelData.materials.size());
        for (auto& shape : m_modelData.shapes)
        {
            ShapeBuffer shapeBuffer{};
            shapeBuffer.vertexBuffer = VertexBuffer(shape.vertexBuffer);
            shapeBuffer.indexBuffer = IndexBuffer{shape.indexBuffer};
            m_shapes[shape.materialIndex].push_back(shapeBuffer);
        }

        // -----------------------------------------------

        // テクスチャ読み込み
        std::map<std::string, ShaderResourceTexture> textureMap{};
        Array<ShaderResourceTexture> diffuseTextureList{};
        for (const auto& texturePath : m_modelData.materialDiffuseTextures)
        {
            if (not textureMap.contains(texturePath))
            {
                const auto texture = texturePath.empty()
                                         ? EnginePresetAsset::GetWhiteTexture()
                                         : ShaderResourceTexture{texturePath};
                textureMap[texturePath] = texture;
                diffuseTextureList.push_back(texture);
            }
            else
            {
                diffuseTextureList.push_back(textureMap[texturePath]);
            }
        }

        // -----------------------------------------------

        m_cb0 = ConstantBuffer<SceneState_b0>{1};

        m_cb1 = ConstantBuffer<ModelMaterial_b>{m_modelData.materials};

        // -----------------------------------------------

        auto descriptorHeapParam = DescriptorHeapParams{
            .table = m_pipelineState.descriptorTable(),
            .materialCounts = {1, m_modelData.materials.size()},
            .descriptors = {CbSrUaSet{{m_cb0}, {}, {}}, CbSrUaSet{{m_cb1}, {diffuseTextureList}, {}}},
        };

        if (params.cb2.has_value())
        {
            descriptorHeapParam.materialCounts.push_back(1);
            descriptorHeapParam.descriptors.push_back(CbSrUaSet{{params.cb2.value()}, {}, {}});
        }

        m_descriptorHeap = DescriptorHeap(descriptorHeapParam);
    }

    void Draw() const
    {
        SceneState_b0 sceneState{};
        sceneState.worldMat = EngineStackState.GetWorldMatrix().mat;
        sceneState.viewMat = EngineStackState.GetViewMatrix().mat;
        sceneState.projectionMat = EngineStackState.GetProjectionMatrix().mat;
        m_cb0.upload(sceneState);

        m_pipelineState.CommandSet();

        // カメラ行列設定
        m_descriptorHeap.CommandSet();
        m_descriptorHeap.CommandSetTable(0);

        if (m_cb2.has_value()) m_descriptorHeap.CommandSetTable(2);

        // マテリアル設定
        for (size_t materialId = 0; materialId < m_shapes.size(); ++materialId)
        {
            m_descriptorHeap.CommandSetTable(1, materialId);

            for (auto& shape : m_shapes[materialId])
            {
                Graphics3D::DrawTriangles(shape.vertexBuffer, shape.indexBuffer);
            }
        }
    }
};

namespace TY
{
    Model::Model(const ModelParams& params) :
        p_impl(std::make_shared<Impl>(params))
    {
    }

    void Model::draw() const
    {
        if (p_impl) p_impl->Draw();
    }
}
