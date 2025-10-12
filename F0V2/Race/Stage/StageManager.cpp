#include "pch.h"
#include "StageManager.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "CB/Skydome.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/CourseBuilder.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"

using namespace Race;

namespace
{
    TextureResource makeGroundPlane(
        const Size& size, int lineSpacing, const UnifiedColor& lineColor, const UnifiedColor& backColor)
    {
        Image image{size, backColor};

        const ColorU8 lineColor2 = lineColor.toColorU8().multiplied(2.0f);

        const ColorU8 backColor2 = backColor.toColorU8().multiplied(0.9f);

        for (int x = 0; x < size.x; x += 2)
        {
            for (int y = 0; y < size.y; y += 2)
            {
                image[Point{x, y}] = backColor2;
            }
        }

        const Size padding = (size % lineSpacing) / 2;

        for (int x = padding.x; x < size.x; x += lineSpacing)
        {
            for (int y = 0; y < size.y; y++)
            {
                image[Point{x, y}] = x == padding.x ? lineColor2 : lineColor.toColorU8();
            }
        }

        for (int y = padding.y; y < size.y; y += lineSpacing)
        {
            for (int x = 0; x < size.x; x++)
            {
                image[Point{x, y}] = y == padding.y ? lineColor2 : lineColor.toColorU8();
            }
        }

        return TextureResource{image};
    }

    // -----------------------------------------------

    void drawBvh(
        const TriangleBvh::Node* node, Immediate3D::LineSet& lineSet, std::pair<int, int> targetRange, int nest = 0)
    {
        if (not node)
        {
            return;
        }

        if (nest > targetRange.second)
        {
            return;
        }

        if (targetRange.first <= nest)
        {
            lineSet.appendAabb(node->aabb());
        }

        if (const auto* branch = node->asBranch())
        {
            drawBvh(branch->left.get(), lineSet, targetRange, nest + 1);
            drawBvh(branch->right.get(), lineSet, targetRange, nest + 1);
        }
    }

    void drawLeafAabb(
        const TriangleBvh::Node* node, Immediate3D::LineSet& lineSet, int targetIndex, int* currentIndex = nullptr)
    {
        if (not node)
        {
            return;
        }

        std::unique_ptr<int> currentIndexPtr{};
        if (not currentIndex)
        {
            currentIndexPtr = std::make_unique<int>(0);
            currentIndex = currentIndexPtr.get();
        }

        if (const auto* leaf = node->asLeaf())
        {
            if (*currentIndex == targetIndex)
            {
                lineSet.appendAabb(node->aabb());
            }

            ++(*currentIndex);

            return;
        }

        if (const auto* branch = node->asBranch())
        {
            drawLeafAabb(branch->left.get(), lineSet, targetIndex, currentIndex);
            drawLeafAabb(branch->right.get(), lineSet, targetIndex, currentIndex);
        }
    }

    void printBvhLeaf(const TriangleBvh::Node* node, int nest = 0)
    {
        if (nest == 0)
        {
            std::cout << "----------------------------------------------- BVH Leaf Information\n";
        }

        if (not node)
        {
            return;
        }

        if (const auto* leaf = node->asLeaf())
        {
            std::cout << std::format("[{}] tris: {}, aabb-volume: {}\n", nest, leaf->tris.size(), leaf->aabb.volume());

            return;
        }

        if (const auto* branch = node->asBranch())
        {
            printBvhLeaf(branch->left.get(), nest + 1);
            printBvhLeaf(branch->right.get(), nest + 1);
        }

        if (nest == 0)
        {
            std::cout << "----------------------------------------------- End BVH Leaf Information\n";
        }
    }

    std::pair s_visibleBvhRange{0, 0};
}

struct StageManager::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<ModelDrawer> m_courseDrawers{};

    float m_courseLength{};

    int m_triangleCount{};
    TriangleBvh m_staticBvh{};

    Array<CourseTriangleAttribute> m_triangleAttributes{};

    void Init()
    {
        auto skydome_b4 = ConstantBufferWrapper<Skydome_b10>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = g_sharedState->fovFarZ;
        skydome_b4.upload();

        m_skydomeDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(g_sharedState->fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(Asset_shader::skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        // -----------------------------------------------

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1000, 1000}, 100, ColorF32{0.5}, ColorF32{0.15});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{100.0f, 100.0f}))
            .setShader(Asset_shader::model)
        };

        // -----------------------------------------------

        CoursePolygoneCollider collider{};
        m_courseLength = 0.0f;
        for (const auto& segment : g_sharedState->courseSegments)
        {
            const auto courseModel = BuildCourseModel(segment, &collider);
            m_courseDrawers.push_back(
                ModelDrawerParams{}
                .setModel(courseModel)
                .setShader(Asset_shader::lambert)
                .setCbv10AndLater({GetRaceContextContent().cb.lambert}));

            m_courseLength += segment.totalLength;
        }

        // -----------------------------------------------

        m_triangleCount = collider.tris.size();
        m_staticBvh = TriangleBvh{collider.tris};

        m_triangleAttributes = std::move(collider.attributes);
    }

private:
    void update() override
    {
        m_skydomeDrawer.uploadWorldMatrix(Mat4x4::Translate(GetRaceContextContent().camera.eyePosition())).draw();

        for (int x = -5; x <= 5; ++x)
        {
            for (int z = -5; z <= 5; ++z)
            {
                m_groundPlaneDrawer
                    .uploadWorldMatrix(Mat4x4::Translate({x * 100.0f, g_sharedState->groundPositionY, z * 100.0f}))
                    .draw();
            }
        }

        for (int i = 0; i < m_courseDrawers.size(); ++i)
        {
            m_courseDrawers[i].draw();
        }

        auto& segments = g_sharedState->courseSegments;

        // コース中心を線分で描画
        {
            Immediate3D::LineSet lineSet{};
            for (int i = 0; i < segments.size(); ++i)
            {
                const auto& segment = segments[i];
                for (int j = 0; j < segment.midwayStrips.size() - 1; ++j)
                {
                    const Float3 d0 = segment.midwayStrips[j].normal;
                    const Float3 d1 = segment.midwayStrips[j + 1].normal;
                    lineSet.appendLine(segment.midwayStrips[j].center + d0, segment.midwayStrips[j + 1].center + d1);
                }
            }

            lineSet.setColor(ColorF32{1.0f, 0.5f, 0.1f})
                   .pushAuto();
        }

        // インデックスをテキスト描画
        const auto worldToScreen = Graphics3D::WorldToScreen();
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            auto p1InScreen = worldToScreen.transformPoint(segment.p1);
            if (not InRange(p1InScreen.z, 0.0f, 1.0f))
            {
                continue;
            }

            Immediate2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
                .setPosition(p1InScreen.xy())
                .pushAuto();
        }

        {
            Immediate3D::LineSet lineSet{};

            drawBvh(m_staticBvh.root().get(), lineSet, s_visibleBvhRange);
            lineSet.setColor(ColorF32{0.3f, 1, 0.3f}).pushAuto();
        }

        ImmediateDrawer::Global().draw();

        debugUI();
    }

    void debugUI()
    {
        ImGui::Begin("Stage Manager");

        ImGui::Text("Triangles Count: %d", m_triangleCount);

        ImGui::DragIntRange2("Visible BVH Range", &s_visibleBvhRange.first, &s_visibleBvhRange.second, 0.1);
        if (ImGui::Button("Expand Visible BVH Range"))
        {
            s_visibleBvhRange.second++;
        }

        if (ImGui::Button("Print BVH Leaf Info to Console"))
        {
            printBvhLeaf(m_staticBvh.root().get());
        }

        ImGui::End();
    }

    void killed() override
    {
        m_children.killEach();
    }

    std::u32string name() const override
    {
        return U"StageManager";
    }
};

namespace Race
{
    StageManager::StageManager() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void StageManager::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    float StageManager::courseLength() const
    {
        return p_impl->m_courseLength;
    }

    TriangleBvh& StageManager::staticBvh()
    {
        return p_impl->m_staticBvh;
    }

    const TriangleBvh& StageManager::staticBvh() const
    {
        return p_impl->m_staticBvh;
    }

    const CourseTriangleAttribute& StageManager::fetchTriangleAttribute(uint64_t index) const
    {
        assert(index < p_impl->m_triangleAttributes.size());
        if (index < p_impl->m_triangleAttributes.size())
        {
            return p_impl->m_triangleAttributes[index];
        }

        static constexpr CourseTriangleAttribute placeholder{};
        return placeholder;
    }

    Array<CourseSegment>& StageManager::courseSegments()
    {
        return g_sharedState->courseSegments;
    }

    const Array<CourseSegment>& StageManager::courseSegments() const
    {
        return g_sharedState->courseSegments;
    }

    std::shared_ptr<GameObjectBase> StageManager::asGameObject() const
    {
        return p_impl;
    }
}
