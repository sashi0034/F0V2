#include "pch.h"

#include "imgui/imgui.h"
#include "Title_PointLight.h"

#include "LivePPAddon.h"
#include "TY/ConstantBuffer.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/Model.h"
#include "TY/ModelLoader.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Shape3D.h"
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
        Float3 rotation{}; // Euler angles in radians

        Mat4x4 getMatrix() const
        {
            return Mat4x4::Identity()
                   .rotatedX(rotation.x)
                   .rotatedY(rotation.y)
                   .rotatedZ(rotation.z)
                   .translated(position);
            // return Mat4x4::RollPitchYaw(rotation).translated(position);
        }
    };

    struct Camera3D
    {
        Float3 position{};
        double yaw{};
        double targetY{};
        Float3 upDirection{0, 1, 0};

        Float3 targetPosition() const
        {
            return Float3{position.x + std::sin(yaw), targetY, position.z + std::cos(yaw)};
        }

        Mat4x4 getMatrix() const
        {
            return Mat4x4::LookAt(position, targetPosition(), upDirection);
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

struct Title_PointLight_impl
{
    Camera3D m_camera{};
    Mat4x4 m_cameraMatrix{m_camera.getMatrix()};

    Mat4x4 m_projectionMat{};

    PixelShader m_modelPS{};
    VertexShader m_modelVS{};

    ConstantBuffer<DirectionLight_cb2> m_planeLight{};

    ConstantBuffer<DirectionLight_cb2> m_directionLight{};

    Model m_planeModel{};

    Model m_gridPlaneModel{};

    Model m_fighterModel{};
    Pose m_fighterPose{};

    Model m_sphereModel{};
    Pose m_spherePose{};

    Title_PointLight_impl()
    {
        resetCamera();

        const PixelShader defaultPS{ShaderParams::PS("asset/shader/model_pixel.hlsl")};
        const VertexShader defaultVS{ShaderParams::VS("asset/shader/model_vertex.hlsl")};

        const PixelShader customPS{ShaderParams{.filename = shader_lambert, .entryPoint = "PS"}};
        const VertexShader customVS{ShaderParams{.filename = shader_lambert, .entryPoint = "VS"}};

        m_planeModel = Model{
            ModelParams{
                .data = ModelLoader::Load("asset/model/dirty_plane.obj"),
                .ps = customPS,
                .vs = customVS,
                .cb2 = m_planeLight
            }
        };

        const auto gridPlaneTexture = makeGridPlane(
            Size{1024, 1024}, 32, ColorF32{0.8}, ColorF32{0.9});
        m_gridPlaneModel = Model{
            ModelParams{}
            .setData(Shape3D::TexturePlane(gridPlaneTexture, Float2{100.0f, 100.0f}))
            .setShaders(defaultPS, defaultVS)
            .setCB2(m_planeLight)
        };

        m_fighterModel = Model{
            ModelParams{
                .data = ModelLoader::Load("asset/model/tie_fighter.obj"),
                .ps = customPS,
                .vs = customVS,
                .cb2 = m_directionLight
            }
        };

        m_fighterPose.position.y = 3.0f;

        m_sphereModel = Model{
            ModelParams{}
            .setData(Shape3D::Sphere(1.0f, ColorF32{1.0, 0.5, 0.3}))
            .setShaders(customPS, customVS)
            .setCB2(m_directionLight)
        };

        m_spherePose.position.y = 5.0f;
    }

    void Update()
    {
        if (not KeyShift.pressed())
        {
            updateCamera();
        }
        else
        {
            const auto poseInput = getPoseInput();
            m_fighterPose.position += poseInput.position.normalized() * 10.0f * System::DeltaTime();
        }

        m_fighterPose.rotation.y += Math::ToRadians(System::DeltaTime() * 90);

        m_directionLight->lightDirection = m_cameraMatrix.forward().normalized();
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
            const Transformer3D t3d{m_fighterPose.getMatrix()};
            m_fighterModel.draw();
        }

        {
            const Transformer3D t3d{m_spherePose.getMatrix()};
            m_sphereModel.draw();
        }

        {
            ImGui::Begin("Camera Info");

            ImGui::Text("Eye Position: (%.2f, %.2f, %.2f)",
                        m_camera.position.x,
                        m_camera.position.y,
                        m_camera.position.z);

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

            ImGui::Text("Rotation (rad): (%.2f, %.2f, %.2f)",
                        m_fighterPose.rotation.x,
                        m_fighterPose.rotation.y,
                        m_fighterPose.rotation.z);

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
        m_camera = {};
    }

    static Pose getPoseInput()
    {
        Pose pose{};

        pose.position.x = (KeyD.pressed() ? 1.0f : 0.0f) - (KeyA.pressed() ? 1.0f : 0.0f);
        pose.position.y = (KeyE.pressed() ? 1.0f : 0.0f) - (KeyX.pressed() ? 1.0f : 0.0f);
        pose.position.z = (KeyW.pressed() ? 1.0f : 0.0f) - (KeyS.pressed() ? 1.0f : 0.0f);

        pose.rotation.x = (KeyRight.pressed() ? 1.0f : 0.0f) - (KeyLeft.pressed() ? 1.0f : 0.0f);
        pose.rotation.y = (KeyUp.pressed() ? 1.0f : 0.0f) - (KeyDown.pressed() ? 1.0f : 0.0f);

        return pose;
    }

    void updateCamera()
    {
        if (KeyR.down())
        {
            resetCamera();
        }

        const auto poseInput = getPoseInput();
        const Float3 moveVector = poseInput.position.normalized();
        const Float3 rotateVector = poseInput.rotation.normalized();

        constexpr double moveSpeed = 10.0f;
        constexpr double rotationSpeed = 50.0f;

        if (not moveVector.isZero())
        {
            const auto forward = m_cameraMatrix.forward();
            const auto df = forward * moveVector.z * moveSpeed * System::DeltaTime();
            m_camera.position += df;

            const auto right = m_cameraMatrix.right();
            const auto dr = right * moveVector.x * moveSpeed * System::DeltaTime();
            m_camera.position += dr;

            const auto up = m_cameraMatrix.up();
            const auto du = up * moveVector.y * moveSpeed * System::DeltaTime();
            m_camera.position += du;

            m_camera.targetY += (df + dr + du).y; // Adjust targetY based on movement
        }

        if (not rotateVector.isZero())
        {
            m_camera.yaw += Math::ToRadians(rotateVector.x * rotationSpeed * System::DeltaTime());

            const auto dy = Abs(m_camera.targetY - m_camera.position.y);
            const auto s = std::sqrt(1 + dy * dy);
            m_camera.targetY += s * Math::ToRadians(rotateVector.y * rotationSpeed * System::DeltaTime());
        }

        m_cameraMatrix = m_camera.getMatrix();
        Graphics3D::SetViewMatrix(m_cameraMatrix);

        m_projectionMat = Mat4x4::PerspectiveFov(
            90.0_deg,
            Scene::Size().horizontalAspectRatio(),
            0.1f,
            100.0f
        );

        Graphics3D::SetProjectionMatrix(m_projectionMat);
    }
};

void Title_PointLight()
{
    Title_PointLight_impl impl{};

    while (System::Update())
    {
#ifdef _DEBUG
        Util::AdvanceLivePP();
#endif

        impl.Update();
    }
}
