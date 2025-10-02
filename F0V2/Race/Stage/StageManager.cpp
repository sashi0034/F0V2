#include "pch.h"
#include "StageManager.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "CB/Skydome.h"
#include "Race/IRaceContext.h"
#include "Race/RaceContextContent.h"
#include "Race/Common/CourseHelper.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/ModelDrawer.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ShapeDrawer.h"
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
}

struct StageManager::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<ModelDrawer> m_courseDrawers{};

    TriangleBvh m_staticBvh{};

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

        for (const auto& segment : g_sharedState->courseSegments)
        {
            m_courseDrawers.push_back(
                ModelDrawerParams{}
                .setModel(BuildCourseModel(segment))
                .setShader(Asset_shader::lambert)
                .setCbv10AndLater({GetRaceContextContent().cb.lambert}));
        }

        // -----------------------------------------------

        Array<Triangle3D> triangles{};
        for (const auto& segment : g_sharedState->courseSegments)
        {
            for (int i = 0; i < segment.midwayStrips.size() - 1; ++i)
            {
                const auto& s0 = segment.midwayStrips[i];
                const auto& s1 = segment.midwayStrips[i + 1];

                triangles.push_back(Triangle3D{s1.leftmost, s0.leftmost, s1.rightmost});
                triangles.push_back(Triangle3D{s1.rightmost, s0.leftmost, s0.rightmost});
            }
        }

        m_staticBvh = TriangleBvh{triangles};
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

        // コース中心を線分で描画
        Shape3D::LineSet lineSet{};
        auto& segments = g_sharedState->courseSegments;
        for (int i = 0; i < segments.size(); ++i)
        {
            const auto& segment = segments[i];
            for (int j = 0; j < segment.midwayPositions.size() - 1; ++j)
            {
                constexpr Float3 d{0, 0.1, 0};
                lineSet.appendLine(segment.midwayPositions[j] + d, segment.midwayPositions[j + 1] + d);
            }
        }

        lineSet.setColor(ColorF32{1.0f, 0.5f, 0.1f})
               .pushAuto();

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

            Shape2D_Text::MPlus1_16_Bitmap(ToUtf32(std::to_string(i)))
                .setPosition(p1InScreen.xy())
                .pushAuto();
        }

        ShapeDrawer::Global().draw();
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

    TriangleBvh& StageManager::staticBvh()
    {
        return p_impl->m_staticBvh;
    }

    const TriangleBvh& StageManager::staticBvh() const
    {
        return p_impl->m_staticBvh;
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
