#include "pch.h"
#include "EditorPlayground.h"

#include "Asset.generated.h"
#include "EditorState.h"
#include "Util/DebugUI.h"
#include "CB/Skydome.h"
#include "GM/DebugService.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
#include "TY/MipmappedDynamicTexture.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/Scene.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/System.h"
#include "TY_Extension/SerializeTransform.h"

using namespace Editor;

using namespace TY;

namespace
{
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

    TextureHandle makeGroundPlane(
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

        return MipmappedDynamicTexture{image.view()};
    }

    constexpr float groundPositionY = -50.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct EditorPlayground::Impl : ActorBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    SimpleCamera3D m_camera{};

    Mat4x4 m_projectionMat{};

    ModelDrawer m_groundPlaneDrawer{};

    Array<SerializeTransform> m_transformList{};

    void init()
    {
        ResetCamera();

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b10>{};
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
            Size{1000, 1000}, 100, ColorF32{0.5}, ColorF32{0.15});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{100.0f, 100.0f}))
            .setShader(Asset_shader::model)
        };
    }

    void ResetCamera()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});
    }

    void ApplyCamera()
    {
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
    }

    void update() override
    {
        m_children.updateEach();

        if (not ImGui::IsAnyItemActive())
        {
            Float3 moveVector = SimpleInput::GetPlayerMovement3D() * (KeyShift.pressed() ? 50.0f : 10.0f);
            moveVector *= g_debugService.cameraSpeed;

            const Float2 rotateVector = Mouse::Drag(MouseM) * Float2{1, -1} * 5.0f;
            m_camera.transform(System::DeltaTime(), moveVector, rotateVector);
        }

        // -----------------------------------------------

        g_editorState->lambert->lightDirection = m_camera.worldMatrix().forward();
        g_editorState->lambert->lightColor = Float3{1.0f, 1.0f, 1.0f};
        g_editorState->lambert.upload();

        ApplyCamera();

        // -----------------------------------------------

        m_skydomeDrawer.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        for (int x = -5; x <= 5; ++x)
        {
            for (int z = -5; z <= 5; ++z)
            {
                m_groundPlaneDrawer
                    .uploadWorldMatrix(Mat4x4::Translate({x * 100.0f, groundPositionY, z * 100.0f}))
                    .draw();
            }
        }

        // -----------------------------------------------

        {
            ImGui::Begin("Editor");

            if (ImGui::Button("Reset Camera"))
            {
                ResetCamera();
            }

            ImGui::End();
        }
    }

    float orderPriority() const override
    {
        return 1000;
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Editor
{
    EditorPlayground::EditorPlayground()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void EditorPlayground::init()
    {
        p_impl->init();
    }

    void EditorPlayground::applyCamera()
    {
        p_impl->ApplyCamera();
    }

    std::shared_ptr<ActorBase> EditorPlayground::asActor() const
    {
        return p_impl;
    }
}
