#include "pch.h"

#include "imgui/imgui.h"
#include "Demo_Basic3D.h"

#include "TY/ConstantBuffer.h"
#include "TY/Gamepad.h"
#include "TY/GameStep.h"
#include "TY/Graphics3D.h"
#include "TY/InlineComponent.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/ModelLoader.h"
#include "TY/Mouse.h"
#include "TY/Random.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Shape3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"

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

    ShaderResourceTexture makeGroundPlane(
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

        return ShaderResourceTexture{image};
    }

    struct CommonResource : IInlineComponent
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

        // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

        GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

        GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};

        ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};

        ConstantBuffer<PhongLight_b4> phongLight{};

        CommonResource()
        {
        }
    };

    InlineComponent<CommonResource> s_resource{};

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct Demo_Basic3D_impl
{
    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBuffer<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    ModelDrawer m_playerDrawer{};
    Pose m_playerPose{};

    ModelDrawer m_mountainDrawer{};

    Demo_Basic3D_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        resetCamera();

        auto skydome_b4 = ConstantBuffer<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Shape3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShaders(s_resource->skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv4AndLater({skydome_b4})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Shape3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShaders(s_resource->model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(s_resource->playerModel)
            .setShaders(s_resource->phong)
            .setCbv4AndLater({s_resource->phongLight})
        };

        m_playerPose.position.y = groundPositionY + 15.0f;

        m_mountainDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(s_resource->mountainModel)
            .setShaders(s_resource->phong)
            .setCbv4AndLater({s_resource->phongLight})
        };
    }

    void Update()
    {
        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

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

        s_resource->phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        s_resource->phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        s_resource->phongLight->eyePosition = m_camera.eyePosition();
        s_resource->phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};

        s_resource->phongLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        m_mountainDrawer.uploadWorldMatrix(Mat4x4::Scale(Float3{5.0})).draw();

        m_playerDrawer.draw();

        {
            ImGui::Begin("Camera");

            ImGui::Text("Eye Position: (%.2f, %.2f, %.2f)",
                        m_camera.eyePosition().x,
                        m_camera.eyePosition().y,
                        m_camera.eyePosition().z);

            const auto targetPosition = m_camera.targetPosition();
            ImGui::Text("Target Position: (%.2f, %.2f, %.2f)",
                        targetPosition.x,
                        targetPosition.y,
                        targetPosition.z);

            ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)",
                        s_resource->phongLight->lightDirection.x,
                        s_resource->phongLight->lightDirection.y,
                        s_resource->phongLight->lightDirection.z);

            ImGui::Separator();

            if (ImGui::Button("Reset Camera"))
            {
                resetCamera();
            }

            ImGui::End();
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
    }

private:
    void resetCamera()
    {
        m_camera.reset(Float3{0.0f, 15.0f, 15.0f});
    }
};

void Demo_Basic3D()
{
    Demo_Basic3D_impl impl{};

    Scene::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
