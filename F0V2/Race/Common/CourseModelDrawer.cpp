#include "pch.h"
#include "CourseModelDrawer.h"

#include "Asset.generated.h"
#include "RaceSharedState.h"
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
            m_shapes.push_back(GenericModelShapeBufferElement{
                .materialIndex = 0,
                .vertexBuffer = VertexBuffer<CourseModelVertex>{data.shape.vertexBuffer},
                .indexBuffer = IndexBuffer{data.shape.indexBuffer},
            });

            // FIXME: シェーダーは b2 を読まないが、GenericModelDrawer が CBV を 1 つ要求するのでダミーを置く
            m_materialCbv.push_back({ConstantBufferObject{Empty}});

            DescriptorList<ShaderResourceObject> textures{};
            for (const auto& texture : g_sharedState->courseTextures)
            {
                textures.push_back(texture);
            }

            m_materialSrv.push_back(textures);
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
        if (modelData.shape.indexBuffer.empty()) return;

        m_impl = GenericModelDrawerParams{}
                 .setModel(std::make_shared<CourseModelBuffer>(modelData))
                 .setVertexInput({
                     // CourseModelVertex のフィールド順と一致させること
                     {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                     {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT},
                     {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT},
                     {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT},
                     {"TEXCOORD", 2, DXGI_FORMAT_R32_UINT},
                     {"TEXCOORD", 3, DXGI_FORMAT_R32_FLOAT},
                 })
                 .setShader(Asset_shader::gbuffer_course1)
                 .setOptions(options);
    }

    void CourseModelDrawer::draw() const
    {
        m_impl.draw();
    }
}
