#include "pch.h"

#include "imgui/imgui.h"
#include "Title_PointLight.h"

#include "LivePPAddon.h"
#include "TY/ConstantBuffer.h"
#include "TY/Gamepad.h"
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
    SimpleCamera3D m_camera{};

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
            m_fighterPose.position += SimpleInput::GetPlayerMovement() * 10.0f * System::DeltaTime();
        }

        m_fighterPose.rotation.y += Math::ToRadians(System::DeltaTime() * 90);

        m_directionLight->lightDirection = m_camera.matrix().forward().normalized();
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

        {
            ImGui::Begin("Gamepad Info");
            const auto& state = MainGamepad.state();

            ImGui::Text("Buttons:");
            ImGui::BeginGroup();
            for (size_t i = 0; i < state.buttons.size(); ++i)
            {
                if (state.buttons[i].pressed)
                {
                    ImGui::SameLine();
                    ImGui::Text("[%zu]", i);
                }
            }

            ImGui::EndGroup();

            ImGui::Text("POV:");
            ImGui::BeginGroup();
            if (state.povUp.pressed)
            {
                ImGui::SameLine();
                ImGui::Text("Up");
            }

            if (state.povDown.pressed)
            {
                ImGui::SameLine();
                ImGui::Text("Down");
            }

            if (state.povLeft.pressed)
            {
                ImGui::SameLine();
                ImGui::Text("Left");
            }

            if (state.povRight.pressed)
            {
                ImGui::SameLine();
                ImGui::Text("Right");
            }

            ImGui::EndGroup();

            ImGui::Text("Axes:");
            ImGui::BeginGroup();
            for (size_t i = 0; i < state.axes.size(); ++i)
            {
                if (state.axes[i] != 0.0f)
                {
                    ImGui::SameLine();
                    ImGui::Text("[%d: %.2f]", i, state.axes[i]);
                }
            }

            ImGui::EndGroup();

            ImGui::End();
        }
    }

    void resetCamera()
    {
        m_camera.reset(Float3{}.withZ(10.0f));
    }

    void updateCamera()
    {
        if (KeyR.down())
        {
            resetCamera();
        }

        m_camera.update();
        Graphics3D::SetViewMatrix(m_camera.matrix());

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
