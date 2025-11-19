#include "pch.h"
#include "StageManager.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "StageStaticCollider.h"
#include "CB/Skydome.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/CourseModelBuilder.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
#include "TY/Graphics3D.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/MipmappedDynamicTexture.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectBase.h"
#include "Util/DebugTomlValue.h"

using namespace Race;

namespace
{
    MipmappedDynamicTexture makeGroundPlane(
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

        return {image};
    }

    // -----------------------------------------------

    // TODO: StaticCollider の方に移動
    void drawBvh(
        const TriangleBvh::NodeReference& node, Immediate3D::LineSet& lineSet, std::pair<int, int> targetRange,
        int nest = 0)
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
            lineSet.appendAabb(node.aabb());
        }

        if (const auto branch = node.asBranch())
        {
            drawBvh(branch.left(), lineSet, targetRange, nest + 1);
            drawBvh(branch.right(), lineSet, targetRange, nest + 1);
        }
    }

    void drawLeafAabb(
        const TriangleBvh::NodeReference& node, Immediate3D::LineSet& lineSet, int targetIndex,
        int* currentIndex = nullptr)
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

        if (const auto leaf = node.asLeaf())
        {
            if (*currentIndex == targetIndex)
            {
                lineSet.appendAabb(node.aabb());
            }

            ++(*currentIndex);

            return;
        }

        if (const auto branch = node.asBranch())
        {
            drawLeafAabb(branch.left(), lineSet, targetIndex, currentIndex);
            drawLeafAabb(branch.right(), lineSet, targetIndex, currentIndex);
        }
    }

    void printBvhLeaf(const TriangleBvh::NodeReference& node, int nest = 0)
    {
        if (nest == 0)
        {
            std::cout << "----------------------------------------------- BVH Leaf Information\n";
        }

        if (not node)
        {
            return;
        }

        if (const auto leaf = node.asLeaf())
        {
            std::cout << std::format("[{}] tris: {}, aabb-volume: {}\n", nest, leaf.triCount(), leaf.aabb().volume());

            return;
        }

        if (const auto branch = node.asBranch())
        {
            printBvhLeaf(branch.left(), nest + 1);
            printBvhLeaf(branch.right(), nest + 1);
        }

        if (nest == 0)
        {
            std::cout << "----------------------------------------------- End BVH Leaf Information\n";
        }
    }

    struct DistanceCache
    {
        struct CachePerStrip
        {
            float distanceFromStart{};
        };

        struct CachePerSegment
        {
            Array<CachePerStrip> strips{};
        };

        Array<CachePerSegment> segments{};
    };

    std::pair s_visibleBvhRange{0, 0};
}

struct StageManager::Impl : GameObjectBase, std::enable_shared_from_this<Impl>, IRaceDrawer
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<ModelDrawer> m_courseDrawers{};

    float m_courseLength{};

    int m_triangleCount{};

    StageStaticCollider m_staticCollider{};

    Array<start_position> m_startPositions{};

    DistanceCache m_distanceCache{};

    void Init()
    {
        GetRaceContext().registerDrawer(shared_from_this());

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b10>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = g_sharedState->farDepth;
        skydome_b4.upload();

        m_skydomeDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(g_sharedState->farDepth, ColorF32{0.5, 0.7, 1.0}))
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
            .setOptions(GraphicsOptions::FromTarget(g_sharedState->gbufferTarget))
            .setShader(Asset_shader::gbuffer_pass)
        };

        // -----------------------------------------------

        m_courseLength = 0.0f;
        m_triangleCount = 0;
        Array<CoursePolygoneCollider> colliders{};
        for (int i = 0; i < g_sharedState->courseSegments.size(); ++i)
        {
            const auto& segment = g_sharedState->courseSegments[i];

            colliders.push_back({});
            const auto courseModel = BuildCourseModel(
                segment,
                {
                    .createStartingLine = i == 0,
                    .outCollider = &colliders.back()
                }
            );

            if (courseModel.isEmpty())
            {
                m_courseDrawers.push_back({});
            }
            else
            {
                m_courseDrawers.push_back(
                    ModelDrawerParams{}
                    .setModel(courseModel)
                    .setOptions(GraphicsOptions::FromTarget(g_sharedState->gbufferTarget))
                    .setShader(Asset_shader::gbuffer_pass));
            }

            m_triangleCount += colliders.back().groundTris.size();

            m_courseLength += segment.totalLength;
        }

        // -----------------------------------------------

        m_staticCollider = StageStaticCollider();
        m_staticCollider.build(colliders);

        buildStartPositions();

        buildDistanceCache();
    }

private:
    void update() override
    {
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

        ImmediateDrawer::Global().draw();

        debugUI();
    }

    void buildStartPositions()
    {
        m_startPositions.clear();

        constexpr int columnsPerLine = 5;
        constexpr int maxMachines = 100;

        const auto& segments = g_sharedState->courseSegments;

        int segmentIndex{static_cast<int>(segments.size()) - 1};
        int stripIndex{static_cast<int>(segments[segmentIndex].midwayStrips.size()) - 1};
        for (int lineId = 0; lineId < maxMachines / columnsPerLine; ++lineId)
        {
            for (int i = 0; i < 2; ++i)
            {
                stripIndex--;
                if (stripIndex < 0)
                {
                    segmentIndex = Modulo(segmentIndex - 1, static_cast<int>(segments.size()));
                    stripIndex = static_cast<int>(segments[segmentIndex].midwayStrips.size()) - 1;
                }
            }

            for (int columnId = 0; columnId < columnsPerLine; ++columnId)
            {
                const auto& targetStrip = segments[segmentIndex].midwayStrips[stripIndex];

                // [0.2, 0.8]
                const float offsetRate =
                    0.2f + static_cast<float>(columnId) / static_cast<float>(columnsPerLine - 1) * 0.6f;

                start_position data;
                data.position = targetStrip.leftmost + (targetStrip.rightmost - targetStrip.leftmost) * offsetRate;
                data.position += targetStrip.normal * 5.0f; // 地面から少し浮かせる

                data.forward = targetStrip.toNext.normalized();

                data.up = targetStrip.normal;

                m_startPositions.push_back(data);
            }
        }

        std::ranges::reverse(m_startPositions);
    }

    void buildDistanceCache()
    {
        float distanceFromStart{};
        for (const auto& segment : g_sharedState->courseSegments)
        {
            DistanceCache::CachePerSegment segmentCache{};
            for (const auto& strip : segment.midwayStrips)
            {
                DistanceCache::CachePerStrip stripCache{};
                stripCache.distanceFromStart = distanceFromStart;
                segmentCache.strips.push_back(stripCache);

                distanceFromStart += strip.toNext.length();
            }

            m_distanceCache.segments.push_back(segmentCache);
        }
    }

    void drawGBuffer() const override
    {
        drawPlaceholderScenery();

        for (int i = 0; i < m_courseDrawers.size(); ++i)
        {
            m_courseDrawers[i].draw();
        }
    }

    void drawPlaceholderScenery() const
    {
#if defined(_DEBUG)
        if (GetDebugTomlValue<bool>("draw_scenery"))
        {
            return;
        }
#else
        return;
#endif
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
    }

    void debugUI()
    {
#if defined(_DEBUG)
        ImGui::Begin("Stage Manager");

        ImGui::Text("Triangles Count: %d", m_triangleCount);

        // TODO: StaticCollider の方に移動
        ImGui::DragIntRange2("Visible BVH Range", &s_visibleBvhRange.first, &s_visibleBvhRange.second, 0.1);
        if (ImGui::Button("Expand Visible BVH Range"))
        {
            s_visibleBvhRange.second++;
        }

        ImGui::End();
#endif
    }

    void killed() override
    {
        m_children.killEach();

        GetRaceContext().unregisterDrawer(this);
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

    StageStaticCollider& StageManager::stageStaticCollider()
    {
        return p_impl->m_staticCollider;
    }

    const StageStaticCollider& StageManager::stageStaticCollider() const
    {
        return p_impl->m_staticCollider;
    }

    Array<CourseSegment>& StageManager::courseSegments()
    {
        return g_sharedState->courseSegments;
    }

    const Array<CourseSegment>& StageManager::courseSegments() const
    {
        return g_sharedState->courseSegments;
    }

    StageManager::start_position StageManager::getStartPosition(int machineId) const
    {
        const int index = Math::Clamp<int>(machineId, 0, static_cast<int>(p_impl->m_startPositions.size() - 1));
        return p_impl->m_startPositions[index];
    }

    float StageManager::getDistanceFromStart(const SegmentAndStrip& pos) const
    {
        return p_impl->m_distanceCache.segments[pos.segmentIndex].strips[pos.stripIndex].distanceFromStart;
    }

    float StageManager::getDistanceFromStart(const LapProgress& pos) const
    {
        float length = pos.lapIndex * p_impl->m_courseLength;
        length += p_impl->m_distanceCache.segments[pos.segmentIndex].strips[pos.stripIndex].distanceFromStart;
        return length;
    }

    std::shared_ptr<GameObjectBase> StageManager::asGameObject() const
    {
        return p_impl;
    }
}
