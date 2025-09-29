#include "pch.h"
#include "DebugPlayground.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "DebugEditorState.h"
#include "DebugUI.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/Intersects2D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Scene.h"
#include "TY/ShapeDrawer.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/System.h"
#include "TY/TextureResource.h"
#include "TY/Utils.h"
#include "TY_Extension/SerializeTransform.h"

using namespace Combat;

using namespace TY;

namespace
{
    struct Skydome_b4
    {
        alignas(16) ColorF32 topColor;
        alignas(16) ColorF32 bottomColor;
        float sphereRadius{};
    };

    struct Pose
    {
        Float3 position{};
        Quaternion rotation{}; // Euler angles in radians

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotated(rotation)
                   .translated(position);
        }

        Float3 eulerAngles() const
        {
            return rotation.eulerAngles();
        }
    };

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

    constexpr float groundPositionY = -50.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct DebugPlayground::Impl : ActorBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    SimpleCamera3D m_camera{};

    Mat4x4 m_projectionMat{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<SerializeTransform> m_transformList{};

    void init()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(Asset_shader::skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1000, 1000}, 100, ColorF32{0.5}, ColorF32{0.25});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{100.0f, 100.0f}))
            .setShader(Asset_shader::model)
        };
    }

    void update() override
    {
        m_children.updateEach();

        if (not ImGui::IsAnyItemActive())
        {
            const Float3 moveVector = SimpleInput::GetPlayerMovement3D() * (KeyShift.pressed() ? 50.0f : 10.0f);

            const Float2 rotateVector = Mouse::Drag(MouseM) * Float2{1, -1} * 5.0f;
            m_camera.transform(System::DeltaTime(), moveVector, rotateVector);
        }

        // -----------------------------------------------

        g_debugEditorState->lambert->lightDirection = m_camera.worldMatrix().forward();
        g_debugEditorState->lambert->lightColor = Float3{1.0f, 1.0f, 1.0f};
        g_debugEditorState->lambert.upload();

        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        {
            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                fovFarZ
            );

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        {
            ImGui::Begin("System Window");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::Text("GPU Memory Usage: %.2f MB", System::GpuMemoryUsage().estimateLocalUsageInMB());

            ImGui::End();
        }

        // -----------------------------------------------

        m_skydomeDrawer.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        for (int x = -1; x <= 1; ++x)
        {
            for (int z = -1; z <= 1; ++z)
            {
                m_groundPlaneDrawer
                    .uploadWorldMatrix(Mat4x4::Translate({x * 100.0f, groundPositionY, z * 100.0f}))
                    .draw();
            }
        }
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Combat
{
    DebugPlayground::DebugPlayground()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void DebugPlayground::init()
    {
        p_impl->init();
    }

    std::shared_ptr<ActorBase> DebugPlayground::asActor() const
    {
        return p_impl;
    }
}
