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
#include "TY/Random.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Shape3D.h"
#include "TY/SimpleInput.h"

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

        PixelShader skydomePS{ShaderParams::PS("asset/shader/skydome.hlsl")};
        VertexShader skydomeVS{ShaderParams::VS("asset/shader/skydome.hlsl")};

        ModelBuffer playerModel{};
        ModelBuffer enemyModel{};

        ModelBuffer missileModel{Shape3D::Sphere(0.5f, ColorF32{0.3, 0.5, 1.0})};

        ConstantBuffer<DirectionLight_cb2> directionLight{};

        CommonResource()
        {
            // モデル
            {
                auto modelData = ModelLoader::Load("asset/model/tie_fighter.obj");

                playerModel = modelData;

                for (int i = 0; i < modelData.materials.size(); ++i)
                {
                    modelData.materials[i].parameters.diffuse =
                        Float3::One() - modelData.materials[i].parameters.diffuse;
                }

                enemyModel = modelData;
            }
        }
    };

    InlineComponent<CommonResource> s_resource{};

    class Internal
    {
    public:
        struct FighterBody;
        class Missile;
        class Player;
        class Camera;
        class Enemy;
    };

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;
}

// 戦闘機
struct Internal::FighterBody
{
    Float3 m_initialPosition{};

    ModelDrawer m_model{};
    Pose m_pose{};

    float m_forwardSpeed{};
    float m_roll{};

    void Init(const ModelBuffer& model, const Float3& initialPosition)
    {
        m_initialPosition = initialPosition;

        m_model = ModelDrawer{
            ModelDrawerParams{}
            .setModel(model)
            .setShaders(s_resource->lambertPS, s_resource->lambertVS)
            .setCB4(s_resource->directionLight)
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

        if (m_pose.position.y < groundPositionY)
        {
            // 地面に衝突
            m_pose.position.y = groundPositionY;
            m_forwardSpeed = 0.0f;
        }

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
        m_model.uploadWorldMatrix(Mat4x4{Quaternion::RotateZ(m_roll)} * matrix).draw();
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

        m_forwardSpeed = 0.0f;

        m_roll = 0.0f;
    }
};

class Internal::Missile
{
public:
    void Init(const Pose& pose)
    {
        m_model = ModelDrawer{
            ModelDrawerParams{}
            .setModel(s_resource->missileModel)
            .setShaders(s_resource->modelPS, s_resource->modelVS)
        };

        m_pose = pose;
    }

    bool Update()
    {
        const auto forward = m_pose.getMatrix().forward();
        m_pose.position += forward * 50.0f * System::DeltaTime();

        m_lifetime += System::DeltaTime();
        return m_lifetime < 5.0f;
    }

    void Draw() const
    {
        m_model.uploadWorldMatrix(m_pose.getMatrix()).draw();
    }

    bool CollideWith(const Pose& targetPose, float radius = 5.0f) const
    {
        const auto distance = (m_pose.position - targetPose.position).lengthSq();
        return distance < radius;
    }

private:
    ModelDrawer m_model{};
    Pose m_pose{};
    float m_lifetime{};
};

class Internal::Player
{
public:
    void Init()
    {
        m_body.Init(s_resource->playerModel, Float3{}.withY(3.0f));

        m_body.m_forwardSpeed = 5.0f;
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

class Internal::Enemy
{
public:
    void Init(const Float3 position)
    {
        m_body.Init(s_resource->enemyModel, position);

        m_body.m_forwardSpeed = 3.0f;

        m_rollInput = Random::Float(-1.0f, 1.0f);
    }

    void Update()
    {
        FighterBody::Input input{};
        input.roll = m_rollInput;
        m_body.Update(input);
    }

    void Draw() const { m_body.Draw(); }

    Pose GetPose() const { return m_body.m_pose; }

private:
    FighterBody m_body{};
    float m_rollInput{0.0f};
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

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBuffer<DirectionLight_cb2> m_planeLight{};

    ModelDrawer m_groundPlaneModel{};

    Internal::Player m_player{};

    Array<Internal::Enemy> m_enemies{};

    ModelDrawer m_sphereModel{};
    Pose m_spherePose{};

    TextureDrawer m_greenAimIcon{};

    Array<Internal::Missile> m_missiles{};

    Demo_AirCombat_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_skydomeModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Shape3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShaders(s_resource->skydomePS, s_resource->skydomeVS)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Shape3D::TexturePlane(groundPlaneTexture, Float2{10000.0f, 10000.0f}))
            .setShaders(s_resource->modelPS, s_resource->modelVS)
        };

        m_player.Init();

        m_enemies.resize(5);
        m_enemies[0].Init(Float3{30.0f, 10.0f, 20.0f});
        m_enemies[1].Init(Float3{-30.0f, 20.0f, 30.0f});
        m_enemies[2].Init(Float3{20.0f, 10.0f, 30.0f});
        m_enemies[3].Init(Float3{-20.0f, 20.0f, 20.0f});
        m_enemies[4].Init(Float3{20.0f, 10.0f, -20.0f});

        m_sphereModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(Shape3D::Sphere(1.0f, ColorF32{1.0, 0.5, 0.3}))
            .setShaders(s_resource->lambertPS, s_resource->lambertVS)
            .setCB4(s_resource->directionLight)
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

        for (auto& enemy : m_enemies)
        {
            enemy.Update();
        }

        {
            if (KeyEnter.down())
            {
                m_missiles.push_back(Internal::Missile{});
                m_missiles.back().Init(m_player.GetPose());
            }

            for (int i = m_missiles.size() - 1; i >= 0; --i)
            {
                if (not m_missiles[i].Update())
                {
                    // ミサイルの寿命が尽きた
                    m_missiles.remove_at(i);
                    continue;
                }

                for (int j = m_enemies.size() - 1; j >= 0; --j)
                {
                    if (m_missiles[i].CollideWith(m_enemies[j].GetPose()))
                    {
                        // ミサイルが敵に当たった
                        m_missiles.remove_at(i);
                        m_enemies.remove_at(j);
                        break;
                    }
                }
            }
        }

        m_camera.Update(m_player);

        {
            m_projectionMat = Mat4x4::PerspectiveFov(
                75.0_deg,
                Scene::Size().horizontalAspectRatio(),
                0.1f,
                fovFarZ
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

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.EyePosition())).draw();

        {
            Pose pose{};
            pose.position.y = groundPositionY;
            m_groundPlaneModel.uploadWorldMatrix(pose.getMatrix()).draw();
        }

        m_player.Draw();

        for (const auto& enemy : m_enemies)
        {
            enemy.Draw();
        }

        for (const auto& missile : m_missiles)
        {
            missile.Draw();
        }

        m_greenAimIcon.as2D().resized({48, 48}).drawAt(Scene::Center());

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
