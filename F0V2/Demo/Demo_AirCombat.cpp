#include "pch.h"

#include "imgui/imgui.h"
#include "Demo_AirCombat.h"

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

    ShaderResourceTexture makeAimIcon(const Size& size, const UnifiedColor& color)
    {
        Image image{size, ColorU8{}};

        for (int y = 0; y < size.y; y += size.y - 1)
        {
            for (int x = 0; x < size.x; ++x)
            {
                image[Point{x, y}] = color;
            }
        }

        for (int y = 0; y < size.y; ++y)
        {
            for (int x = 0; x < size.x; x += size.x - 1)
            {
                image[Point{x, y}] = color;
            }
        }

        return ShaderResourceTexture{image};
    }

    struct CommonResource : IInlineComponent
    {
        PixelShader default2d_ps{ShaderParams::PS("asset/shader/default2d.hlsl")};
        VertexShader default2d_vs{ShaderParams::VS("asset/shader/default2d.hlsl")};

        PixelShader modelPS{ShaderParams::PS("asset/shader/model_pixel.hlsl")};
        VertexShader modelVS{ShaderParams::VS("asset/shader/model_vertex.hlsl")};

        PixelShader lambertPS{ShaderParams::PS("asset/shader/lambert.hlsl")};
        VertexShader lambertVS{ShaderParams::VS("asset/shader/lambert.hlsl")};

        ModelData fighterModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ConstantBuffer<DirectionLight_cb2> directionLight{};
    };

    InlineComponent<CommonResource> s_resource{};

    class Internal
    {
    public:
        struct FighterBody;
        class Player;
        class Camera;
    };
}

struct Internal::FighterBody
{
    Float3 m_initialPosition{};

    ModelDrawer m_model{};
    Pose m_pose{};

    float m_forwardSpeed{};
    float m_roll{};

    void Init(const ModelData& model, const Float3& initialPosition)
    {
        m_initialPosition = initialPosition;

        m_model = ModelDrawer{
            ModelDrawerParams{}
            .setData(model)
            .setShaders(s_resource->lambertPS, s_resource->lambertVS)
            .setCB2(s_resource->directionLight)
        };

        ResetParameters();
    }

    struct Input
    {
        float roll{}; // 左右の傾き [-1.0, 1.0]
        float pitch{}; // 上昇・下降 [-1.0, 1.0]
        float speed{}; // 前進・後退の速度 [-1.0, 1.0]
    };

    void Update(const Input& input)
    {
        // ロール更新
        const float targetRoll = input.roll * 15.0_deg;
        for (const auto dt : StandardStep_60Hz())
        {
            m_roll = Math::Lerp(m_roll, targetRoll, 10.0f * dt);
        }

        // 速度更新
        m_forwardSpeed += 5.0f * input.speed * System::DeltaTime();
        m_forwardSpeed = Math::Clamp(m_forwardSpeed, 0.0f, 50.0f);

        // 機体正面方向へ前進
        const auto playerMatrix = m_pose.getMatrix();
        const auto playerForward = playerMatrix.forward();
        m_pose.position += playerForward * m_forwardSpeed * System::DeltaTime();

        // ヨー回転
        m_pose.rotation *=
            Quaternion::RotateY(-m_roll * 2.0f * System::DeltaTime());

        // ピッチ回転
        m_pose.rotation *=
            Quaternion{playerMatrix.right(), input.pitch * 1.0f * System::DeltaTime()};
    }

    void Draw() const
    {
        const auto matrix = m_pose.getMatrix();
        const Transformer3D t3d{Mat4x4{Quaternion::RotateZ(m_roll)} * matrix};

        m_model.draw();
    }

    void DebugGUI()
    {
        ImGui::Text("Position: (%.2f, %.2f, %.2f)",
                    m_pose.position.x,
                    m_pose.position.y,
                    m_pose.position.z);

        const auto rotation = m_pose.eulerAngles();
        ImGui::Text("Rotation (rad): (%.2f, %.2f, %.2f)", rotation.x, rotation.y, rotation.z);

        const auto forward = m_pose.getMatrix().forward();
        ImGui::Text("Forward: (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);

        if (ImGui::Button("Reset"))
        {
            ResetParameters();
        }
    }

    void ResetParameters()
    {
        m_pose = {};
        m_pose.position = m_initialPosition;

        m_forwardSpeed = 5.0f;

        m_roll = 0.0f;
    }
};

class Internal::Player
{
public:
    void Init()
    {
        m_body.Init(s_resource->fighterModel, Float3{}.withY(3.0f));
    }

    void Update()
    {
        FighterBody::Input input{};
        input.roll = -SimpleInput::GetPlayerMovement2D().x;
        input.pitch = -SimpleInput::GetPlayerMovement2D().y;
        input.speed = IsGamepadPreferred()
                          ? (MainGamepad.rt().pressed - MainGamepad.lt().pressed)
                          : (KeyUp.pressed() - KeyDown.pressed());
        m_body.Update(input);
    }

    void Draw() const { m_body.Draw(); }

    void DebugUI()
    {
        ImGui::Begin("Player Info");

        ImGui::Text("Speed: %.2f", m_body.m_forwardSpeed);

        ImGui::Text("Altitude: %.2f", m_body.m_pose.position.y);

        ImGui::Separator();

        ImGui::Text("%s", IsGamepadPreferred() ? "Gamepad" : "Keyboard & Mouse");

        m_body.DebugGUI();

        ImGui::End();
    }

    Pose GetPose() const { return m_body.m_pose; }

private:
    FighterBody m_body{};
};

class Internal::Camera
{
public:
    void Update(const Player& player)
    {
        const auto playerPose = player.GetPose();

        const auto upDirection = playerPose.getMatrix().up();

        m_targetPosition = playerPose.position + upDirection * 4.0f;

        const auto playerForward = playerPose.getMatrix().forward().normalized();
        m_eyePosition = m_targetPosition - playerForward * 8.0f;

        m_viewMatrix = Mat4x4::LookAt(
            m_eyePosition,
            m_targetPosition,
            upDirection
        );

        m_worldMatrix = m_viewMatrix.transposed();

        Graphics3D::SetViewMatrix(m_viewMatrix);
    }

    Float3 EyePosition() const { return m_eyePosition; }

    Float3 TargetPosition() const { return m_targetPosition; }

    const Mat4x4& ViewMatrix() const { return m_viewMatrix; }

    const Mat4x4& WorldMatrix() const { return m_worldMatrix; }

private:
    Float3 m_eyePosition{};
    Float3 m_targetPosition{};

    Mat4x4 m_viewMatrix{};
    Mat4x4 m_worldMatrix{};
};

struct Demo_AirCombat_impl
{
    Internal::Camera m_camera{};

    Mat4x4 m_projectionMat{};

    ConstantBuffer<DirectionLight_cb2> m_planeLight{};

    ModelDrawer m_planeModel{};

    ModelDrawer m_gridPlaneModel{};

    Internal::Player m_player{};

    ModelDrawer m_sphereModel{};
    Pose m_spherePose{};

    TextureDrawer m_greenAimIcon{};

    Demo_AirCombat_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_planeModel = ModelDrawer{
            ModelDrawerParams{}
            .setData(ModelLoader::Load("asset/model/dirty_plane.obj"))
            .setShaders(s_resource->lambertPS, s_resource->lambertVS)
            .setCB2(m_planeLight)
        };

        const auto gridPlaneTexture = makeGridPlane(
            Size{1024, 1024}, 32, ColorF32{0.8}, ColorF32{0.9});
        m_gridPlaneModel = ModelDrawer{
            ModelDrawerParams{}
            .setData(Shape3D::TexturePlane(gridPlaneTexture, Float2{100000.0f, 100000.0f}))
            .setShaders(s_resource->modelPS, s_resource->modelVS)
        };

        m_player.Init();

        m_sphereModel = ModelDrawer{
            ModelDrawerParams{}
            .setData(Shape3D::Sphere(1.0f, ColorF32{1.0, 0.5, 0.3}))
            .setShaders(s_resource->lambertPS, s_resource->lambertVS)
            .setCB2(s_resource->directionLight)
        };

        m_spherePose.position.y = 5.0f;

        m_greenAimIcon = TextureDrawer{
            TextureDrawerParams{}
            .setShaders(s_resource->default2d_ps, s_resource->default2d_vs)
            .setSource(makeAimIcon(Size{32, 32}, ColorF32{0.3f, 1.0f, 0.3f}).getResource())
        };
    }

    void Update()
    {
        m_player.Update();

        m_camera.Update(m_player);

        {
            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                1000.0f
            );

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        s_resource->directionLight->lightDirection = m_camera.WorldMatrix().forward().normalized();
        s_resource->directionLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        s_resource->directionLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        {
            m_planeModel.draw();
        }

        {
            Pose pose{};
            pose.position.y = -10.0f;
            const Transformer3D t3d{pose.getMatrix()};
            m_gridPlaneModel.draw();
        }

        m_player.Draw();

        {
            const Transformer3D t3d{m_spherePose.getMatrix()};
            m_sphereModel.draw();
        }

        {
            m_greenAimIcon.as2D().resized({48, 48}).drawAt(Scene::Center());
        }

        {
            ImGui::Begin("Camera Info");

            ImGui::Text("Eye Position: (%.2f, %.2f, %.2f)",
                        m_camera.EyePosition().x,
                        m_camera.EyePosition().y,
                        m_camera.EyePosition().z);

            const auto targetPosition = m_camera.TargetPosition();
            ImGui::Text("Target Position: (%.2f, %.2f, %.2f)",
                        targetPosition.x,
                        targetPosition.y,
                        targetPosition.z);

            ImGui::Text("Light Direction: (%.2f, %.2f, %.2f)",
                        s_resource->directionLight->lightDirection.x,
                        s_resource->directionLight->lightDirection.y,
                        s_resource->directionLight->lightDirection.z);

            ImGui::End();
        }

        m_player.DebugUI();

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
