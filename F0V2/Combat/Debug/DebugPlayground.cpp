#include "pch.h"
#include "DebugPlayground.h"

#include "Asset.generated.h"
#include "Asset0.h"
#include "ColorPalette.h"
#include "DebugUI.h"
#include "TY/ActorContainer.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"
#include "TY/ModelDrawer.h"
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
    struct LambertLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct PhongLight_b4
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
        alignas(16) Float3 eyePosition{};
        alignas(16) Float3 ambientColor{};
    };

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
                image[Point{x, y}] = lineColor;
            }
        }

        for (int y = padding.y; y < size.y; y += lineSpacing)
        {
            for (int x = 0; x < size.x; x++)
            {
                image[Point{x, y}] = lineColor;
            }
        }

        return TextureResource{image};
    }

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct DebugPlayground::Impl : ActorBase
{
    ActorContainer m_children{};

    ModelDrawer m_skydomeDrawer{};

    ModelDrawer m_playerDrawer{};

    Pose m_playerPose{};

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

        ConstantBufferWrapper<PhongLight_b4> phongLight{};

        phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        phongLight->eyePosition = m_camera.eyePosition();
        phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};
        phongLight.upload();

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Asset_model::tie_fighter)
            .setShader(Asset_shader::phong)
            .setCbv10AndLater({phongLight})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(Asset_shader::model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));
    }

    void update() override
    {
        m_children.updateEach();

        // ウィンドウ内でドック可能にする
        // ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        // -----------------------------------------------

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
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::End();
        }

        // -----------------------------------------------

        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

        if (not ImGui::IsAnyItemActive())
        {
            if (not KeyShift.pressed())
            {
                m_camera.transformBySimpleInput();
            }
            else
            {
                const auto matrix = m_playerPose.getMatrix();
                const auto forward = matrix.forward();
                const auto right = matrix.right();
                const auto up = matrix.up();

                const auto moveInput = SimpleInput::GetPlayerMovement3D();
                constexpr auto speed = 10.0f;
                m_playerPose.position += forward * moveInput.z * speed * System::DeltaTime();
                m_playerPose.position += right * moveInput.x * speed * System::DeltaTime();
                m_playerPose.position += up * moveInput.y * speed * System::DeltaTime();

                const auto rotateInput = SimpleInput::GetCameraRotation();
                constexpr auto rotationSpeed = 2.0f;
                m_playerPose.rotation *= Quaternion::RotateY(rotateInput.x * rotationSpeed * System::DeltaTime());
            }
        }

        // -----------------------------------------------

        m_skydomeDrawer.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_playerDrawer.draw();

        m_groundPlaneDrawer.draw();
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
