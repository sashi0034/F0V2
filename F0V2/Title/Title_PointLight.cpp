#include "pch.h"

#include "imgui.h"
#include "Title_Phong.h"

#include "LivePPAddon.h"
#include "TY/ConstantBuffer.h"
#include "TY/Graphics3D.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/Math.h"
#include "TY/Model.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Transformer3D.h"

using namespace TY;

namespace
{
    struct DirectionLight_cb2
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    struct CameraTransform
    {
        Float3 position{};
        Float3 rotation{}; // Euler angles in radians

        Mat4x4 viewMatrix() const
        {
            return Mat4x4::Identity()
                   .rotatedX(rotation.x)
                   .rotatedY(rotation.y)
                   .rotatedZ(rotation.z)
                   .translated(position);
            // return Mat4x4::RollPitchYaw(rotation).translated(position);
        }
    };

    const std::string shader_lambert = "asset/shader/lambert.hlsl";
}

struct Title_PointLight_impl
{
    Mat4x4 m_worldMat{};

    CameraTransform m_camera{};

    Mat4x4 m_projectionMat{};

    PixelShader m_modelPS{};
    VertexShader m_modelVS{};

    ConstantBuffer<DirectionLight_cb2> m_planeLight{};

    ConstantBuffer<DirectionLight_cb2> m_directionLight{};

    Model m_planeModel{};
    Model m_robotModel{};

    Title_PointLight_impl()
    {
        m_worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

        resetCamera();

        m_modelPS = PixelShader{ShaderParams{.filename = shader_lambert, .entryPoint = "PS"}};
        m_modelVS = VertexShader{ShaderParams{.filename = shader_lambert, .entryPoint = "VS"}};

        m_planeModel = Model{
            ModelParams{
                .filename = "asset/model/dirty_plane.obj",
                .ps = m_modelPS,
                .vs = m_modelVS,
                .cb2 = m_planeLight
            }
        };

        m_robotModel = Model{
            ModelParams{
                .filename = "asset/model/robot_head.obj", // "asset/model/cinnamon.obj"
                .ps = m_modelPS,
                .vs = m_modelVS,
                .cb2 = m_directionLight
            }
        };
    }

    void Update()
    {
        updateCamera();

        m_worldMat = m_worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));

        {
            m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
            m_planeLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
            m_planeLight.upload();

            m_planeModel.draw();
        }

        {
            const Transformer3D t3d{m_worldMat};

            m_directionLight->lightDirection = m_camera.viewMatrix().forward().normalized();
            m_directionLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
            m_directionLight.upload();

            m_robotModel.draw();
        }

        {
            ImGui::Begin("Camera Info");

            ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                        m_camera.position.x,
                        m_camera.position.y,
                        m_camera.position.z);

            ImGui::Text("Rotation (rad): (%.2f, %.2f, %.2f)",
                        m_camera.rotation.x,
                        m_camera.rotation.y,
                        m_camera.rotation.z);

            ImGui::Text("Rotation (deg): (%.1f, %.1f, %.1f)",
                        Math::ToDegrees(m_camera.rotation.x),
                        Math::ToDegrees(m_camera.rotation.y),
                        Math::ToDegrees(m_camera.rotation.z));

            ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)",
                        m_directionLight->lightDirection.x,
                        m_directionLight->lightDirection.y,
                        m_directionLight->lightDirection.z);

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
        m_camera.position = Float3{0.0f, 0.0f, 5.0f};
        m_camera.rotation = Float3{-Math::PiF / 4.0f, 0.0f, 0.0f};
    }

    void updateCamera()
    {
        if (KeyR.down())
        {
            resetCamera();
        }

        constexpr float moveSpeed = 10.0f;
        constexpr float rotationSpeed = 50.0f;

        Float3 moveVector{};
        moveVector.x = (KeyA.pressed() ? 1.0f : 0.0f) - (KeyD.pressed() ? 1.0f : 0.0f);
        moveVector.y = (KeyX.pressed() ? 1.0f : 0.0f) - (KeyE.pressed() ? 1.0f : 0.0f);
        moveVector.z = (KeyS.pressed() ? 1.0f : 0.0f) - (KeyW.pressed() ? 1.0f : 0.0f);
        if (not moveVector.isZero())
        {
            m_camera.position += moveVector * moveSpeed * System::DeltaTime();
        }

        Float2 rotateVector{};
        rotateVector.x = (KeyRight.pressed() ? 1.0f : 0.0f) - (KeyLeft.pressed() ? 1.0f : 0.0f);
        rotateVector.y = (KeyDown.pressed() ? 1.0f : 0.0f) - (KeyUp.pressed() ? 1.0f : 0.0f);
        if (not rotateVector.isZero())
        {
            m_camera.rotation.x += Math::ToRadians(-rotateVector.y * rotationSpeed * System::DeltaTime());
            m_camera.rotation.y += Math::ToRadians(rotateVector.x * rotationSpeed * System::DeltaTime());
        }

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
