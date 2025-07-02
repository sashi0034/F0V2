#include "pch.h"

#include "imgui/imgui.h"
#include "Demo_AirCombat.h"

#include "TY/ConstantBuffer.h"
#include "TY/Gamepad.h"
#include "TY/GameStep.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/ModelLoader.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Shape3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"
#include "TY/Transformer3D.h"

using namespace TY;

namespace
{
    struct DirectionLight_cb2
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
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

    ShaderResourceTexture makeGridPlane(
        const Size& size, int lineSpacing, const UnifiedColor& lineColor, const UnifiedColor& backColor)
    {
        Image image{size, backColor};
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

    const std::string shader_lambert = "asset/shader/lambert.hlsl";
}

struct Demo_AirCombat_impl
{
    SimpleCamera3D m_camera{};

    Mat4x4 m_projectionMat{};

    PixelShader m_modelPS{};
    VertexShader m_modelVS{};

    ConstantBuffer<DirectionLight_cb2> m_planeLight{};

    ConstantBuffer<DirectionLight_cb2> m_directionLight{};

    ModelDrawer m_planeModel{};

    ModelDrawer m_gridPlaneModel{};

    ModelDrawer m_fighterModel{};
    Pose m_fighterPose{};

    float m_playerYaw{};

    ModelDrawer m_sphereModel{};
    Pose m_spherePose{};

    Demo_AirCombat_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        resetCamera();

        const PixelShader defaultPS{ShaderParams::PS("asset/shader/model_pixel.hlsl")};
        const VertexShader defaultVS{ShaderParams::VS("asset/shader/model_vertex.hlsl")};

        const PixelShader customPS{ShaderParams{.filepath = shader_lambert, .entryPoint = "PS"}};
        const VertexShader customVS{ShaderParams{.filepath = shader_lambert, .entryPoint = "VS"}};

        m_planeModel = ModelDrawer{
            ModelDrawerParams{
                .data = ModelLoader::Load("asset/model/dirty_plane.obj"),
                .ps = customPS,
                .vs = customVS,
                .cb2 = m_planeLight
            }
        };

        const auto gridPlaneTexture = makeGridPlane(
            Size{1024, 1024}, 32, ColorF32{0.8}, ColorF32{0.9});
        m_gridPlaneModel = ModelDrawer{
            ModelDrawerParams{}
            .setData(Shape3D::TexturePlane(gridPlaneTexture, Float2{100.0f, 100.0f}))
            .setShaders(defaultPS, defaultVS)
            .setCB2(m_planeLight)
        };

        m_fighterModel = ModelDrawer{
            ModelDrawerParams{
                .data = ModelLoader::Load("asset/model/tie_fighter.obj"),
                .ps = customPS,
                .vs = customVS,
                .cb2 = m_directionLight
            }
        };

        m_fighterPose.position.y = 3.0f;

        m_sphereModel = ModelDrawer{
            ModelDrawerParams{}
            .setData(Shape3D::Sphere(1.0f, ColorF32{1.0, 0.5, 0.3}))
            .setShaders(customPS, customVS)
            .setCB2(m_directionLight)
        };

        m_spherePose.position.y = 5.0f;
    }

    void Update()
    {
        updateCamera();

        updatePlayer();

        m_directionLight->lightDirection = m_camera.worldMatrix().forward().normalized();
        m_directionLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        m_directionLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        {
            m_planeModel.draw();
        }

        {
            Pose pose{};
            pose.position.y = -10.0f;
            const Transformer3D t3d{pose.getMatrix()};
            m_gridPlaneModel.draw();
        }

        {
            const auto matrix = m_fighterPose.getMatrix();
            const Transformer3D t3d{Mat4x4{Quaternion::RotateZ(m_playerYaw)} * matrix};

            m_fighterModel.draw();
        }

        {
            const Transformer3D t3d{m_spherePose.getMatrix()};
            m_sphereModel.draw();
        }

        {
            ImGui::Begin("Camera Info");

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
                        m_directionLight->lightDirection.x,
                        m_directionLight->lightDirection.y,
                        m_directionLight->lightDirection.z);

            ImGui::End();
        }

        {
            ImGui::Begin("Fighter Pose");

            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        m_fighterPose.position.x,
                        m_fighterPose.position.y,
                        m_fighterPose.position.z);

            const auto rotation = m_fighterPose.eulerAngles();
            ImGui::Text("Rotation (rad): (%.2f, %.2f, %.2f)", rotation.x, rotation.y, rotation.z);

            const auto forward = m_fighterPose.getMatrix().forward();
            ImGui::Text("Forward: (%.2f, %.2f, %.2f)",
                        forward.x,
                        forward.y,
                        forward.z);

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

    void resetCamera()
    {
        m_camera.reset(Float3{}.withZ(10.0f));
    }

    void updatePlayer()
    {
        const auto playerMatrix = m_fighterPose.getMatrix();
        const auto playerForward = playerMatrix.forward();
        const auto playerRight = playerMatrix.right();
        m_fighterPose.position += playerForward * -SimpleInput::GetPlayerMovement2D().y * 10.0f * System::DeltaTime();
        m_fighterPose.position += playerRight * SimpleInput::GetPlayerMovement2D().x * 10.0f * System::DeltaTime();

        m_fighterPose.rotation *=
            Quaternion::RotateY(SimpleInput::GetCameraRotation().x * 1.0f * System::DeltaTime());

        m_fighterPose.rotation *=
            Quaternion{playerMatrix.right(), SimpleInput::GetCameraRotation().y * 1.0f * System::DeltaTime()};

        const float targetYaw = -SimpleInput::GetCameraRotation().x * 15.0_deg;
        for (const auto dt : StandardStep_60Hz())
        {
            m_playerYaw = Math::Lerp(m_playerYaw, targetYaw, 10.0f * dt);
        }
    }

    void updateCamera()
    {
        const auto cameraTarget = m_fighterPose.position;
        const auto playerForward = m_fighterPose.getMatrix().forward().withY(0.0f).normalized();
        const auto cameraEye = cameraTarget - playerForward * 10.0f + Float3{0, -0.5f, 0};
        m_camera.setEyeAndTarget(cameraEye, cameraTarget);
        Graphics3D::SetViewMatrix(m_camera.viewMatrix());

        m_projectionMat = Mat4x4::PerspectiveFov(
            90.0_deg,
            Scene::Size().horizontalAspectRatio(),
            0.1f,
            100.0f
        );

        Graphics3D::SetProjectionMatrix(m_projectionMat);
    }
};

void Demo_AirCombat()
{
    Demo_AirCombat_impl impl{};

    Scene::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
