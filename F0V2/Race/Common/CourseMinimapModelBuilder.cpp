#include "pch.h"
#include "CourseMinimapModelBuilder.h"

using namespace Race;

namespace Race
{
    void CourseMinimapModelBuilder::pushGroundQuad(
        const CourseMinimapFace& l0,
        const CourseMinimapFace& r0,
        const CourseMinimapFace& l1,
        const CourseMinimapFace& r1)
    {
        const auto v_offset = static_cast<uint16_t>(m_vertices.size());

        // NOTE: uint16_t のインデックスに収まらなくなったら諦めて打ち切る
        //       (本体の半分の頂点数なので、本体が収まっている限り起きないはず)
        if (m_vertices.size() + 4 > std::numeric_limits<uint16_t>::max())
        {
            assert(false && "CourseMinimapModelBuilder: too many vertices.");
            return;
        }

        m_vertices.push_back(ModelVertex{r1.position, r1.normal, Float2{0.0f, 1.0f}});
        m_vertices.push_back(ModelVertex{l1.position, l1.normal, Float2{1.0f, 1.0f}});
        m_vertices.push_back(ModelVertex{r0.position, r0.normal, Float2{0.0f, 0.0f}});
        m_vertices.push_back(ModelVertex{l0.position, l0.normal, Float2{1.0f, 0.0f}});

        // 巻き順は pushGroundFaces の表面と揃えてある
        m_indices.push_back(v_offset);
        m_indices.push_back(v_offset + 2);
        m_indices.push_back(v_offset + 1);
        m_indices.push_back(v_offset + 1);
        m_indices.push_back(v_offset + 2);
        m_indices.push_back(v_offset + 3);
    }

    bool CourseMinimapModelBuilder::isEmpty() const
    {
        return m_indices.empty();
    }

    ModelBuffer CourseMinimapModelBuilder::build() const
    {
        if (isEmpty())
        {
            return {};
        }

        ModelData model{};
        model.shapes.push_back(ModelShape{m_vertices, m_indices, 0});

        // NOTE: minimap.hlsl はマテリアルを参照しないが、ModelShape が参照先を要求するので 1 本だけ置く
        model.materials.push_back({
            .name = "minimap",
            .parameters = {
                .albedo = Float3::One(),
            },
        });

        return model;
    }
}
