#include "pch.h"
#include "CourseModelDrawer.h"

#include "Asset.generated.h"
#include "TY/GenericModelBuffer.h"
#include "TY/IndexBuffer.h"
#include "TY/VertexBuffer.h"

using namespace Race;

namespace
{
    class CourseModelBuffer final : public IGenericModelBuffer
    {
    public:
        explicit CourseModelBuffer(const CourseModelData& data)
        {
            for (const auto& shape : data.shapes)
            {
                m_shapes.push_back(GenericModelShapeBufferElement{
                    .materialIndex = shape.materialIndex,
                    .vertexBuffer = VertexBuffer<CourseModelVertex>{shape.vertexBuffer},
                    .indexBuffer = IndexBuffer{shape.indexBuffer},
                });
            }

            for (const auto& material : data.materials)
            {
                // FIXME: シェーダーは b2 を読まないが、GenericModelDrawer が CBV を 1 つ要求するのでダミーを置く
                m_materialCbv.push_back({ConstantBufferObject{Empty}});

                // srvCountPerMaterial を 1 に固定するため、テクスチャが無くても必ず 1 つ入れる
                m_materialSrv.push_back({material.texture});
            }
        }

        int shapeCount() const override
        {
            return static_cast<int>(m_shapes.size());
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            return m_shapes[index];
        }

        int materialCount() const override
        {
            return static_cast<int>(m_materialCbv.size());
        }

        const MaterialList<DescriptorList<ConstantBufferObject>>& materialCbv() const override
        {
            return m_materialCbv;
        }

        MaterialList<DescriptorList<ShaderResourceObject>> materialSrv() const override
        {
            return m_materialSrv;
        }

    private:
        Array<GenericModelShapeBufferElement> m_shapes{};

        MaterialList<DescriptorList<ConstantBufferObject>> m_materialCbv{};

        MaterialList<DescriptorList<ShaderResourceObject>> m_materialSrv{};
    };
}

namespace Race
{
    CourseModelDrawer::CourseModelDrawer(const CourseModelData& modelData, const GraphicsOptions& options)
    {
        if (modelData.shapes.empty()) return;

        m_impl = GenericModelDrawerParams{}
                 .setModel(std::make_shared<CourseModelBuffer>(modelData))
                 .setVertexInput({
                     // CourseModelVertex のフィールド順と一致させること
                     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                     {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                     {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                     {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT},
                     {"TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT},
                 })
                 .setShader(Asset_shader::gbuffer_course1)
                 .setOptions(options);
    }

    void CourseModelDrawer::draw() const
    {
        m_impl.draw();
    }
}
