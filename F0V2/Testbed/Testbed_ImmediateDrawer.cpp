#include "pch.h"

#include "imgui/imgui.h"
#include "Testbed_ImmediateDrawer.h"

#include "TY/BitmapFont.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
#include "TY/Gamepad.h"
#include "TY/GameTime.h"
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
#include "TY/Screen.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/ImmediateDrawer.h"
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

    DynamicTexture makeGroundPlane(
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

        return {image};
    }

    constexpr float groundPositionY = -10.0f;

    constexpr float fovFarZ = 1000.0f;
}

struct Testbed_ImmediateDrawer_impl
{
    struct
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

        // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

        GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

        GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};
    } m_shaders;

    struct
    {
        ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};
    } m_models;

    struct
    {
        ConstantBufferWrapper<PhongLight_b4> phongLight{};
    } m_cb;

    BitmapFont m_zxProtoBitmap{"asset/font/0xProto/0xProto-Regular.ttf", 32};

    BitmapFont m_rocknRollOneBitmap{"asset/font/RocknRoll/RocknRollOne-Regular.ttf", 32};

    SdfFont m_rocknRollOneSdf{"asset/font/RocknRoll/RocknRollOne-Regular.ttf", 32};

    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    ModelDrawer m_playerDrawer{};
    Pose m_playerPose{};

    ModelDrawer m_mountainDrawer{};

    RenderTarget m_miniMap{};

    TextureDrawer m_miniMapDrawer{};

    Testbed_ImmediateDrawer_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        resetCamera();

        auto skydome_b4 = ConstantBufferWrapper<Skydome_b4>{};
        skydome_b4->topColor = ColorF32{0.3f, 0.0f, 1.0f};
        skydome_b4->bottomColor = ColorF32{1.0f, 1.0f, 1.0f};
        skydome_b4->sphereRadius = fovFarZ;
        skydome_b4.upload();

        m_skydomeModel = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Sphere(fovFarZ, ColorF32{0.5, 0.7, 1.0}))
            .setShader(m_shaders.skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(m_shaders.model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.playerModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_playerPose.position.y = groundPositionY + 15.0f;

        m_mountainDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.mountainModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_miniMap = RenderTarget{
            RenderTargetParams{}
            .setTarget(RtvParams{}.setSize({256, 256}).setClearColor(ColorF32{0.0f, 1.0f}))
        };

        m_miniMapDrawer = TextureDrawer{
            TextureDrawerParams{}
            .setShader(m_shaders.default2d)
            .setTexture(m_miniMap.getFrontTarget())
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
                Screen::Size().horizontalAspectRatio(),
                0.1f,
                fovFarZ
            );

            Graphics3D::SetProjectionMatrix(m_projectionMat);
        }

        m_cb.phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        m_cb.phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        m_cb.phongLight->eyePosition = m_camera.eyePosition();
        m_cb.phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};

        m_cb.phongLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        m_mountainDrawer.uploadWorldMatrix(Mat4x4::Scale(Float3{5.0})).draw();

        m_playerDrawer.draw();

        // -----------------------------------------------

        ImmediateDrawer::Global()
            .push(Immediate3D::Line{
                    Float3{0, -5, 0},
                    Float3{10, 30, 10}
                }.setColor(ColorF32{1.0f, 0.3f, 0.7f})
            )
            .push(Immediate2D::Rect{RectF{10, 10, 100, 50}})
            .push(Immediate2D::Rect{RectF{1000, 10, 100, 50}}
                .setColor(ColorF32{1.0f, 0.3f, 0.7f, 0.5f})
            )
            .push(Immediate2D::Line{Float2{100, 200}, Float2{200, 300}}
                  .setThickness(5.0f)
                  .setColor(ColorF32{1.0f, 0.9f, 0.3f})
                  .asDotLine(InGameElapsedTime() * 50.0f)
            )
            .push(Immediate2D::Line{Float2{200, 200}, Float2{300, 400}}
                  .setThickness(10.0f)
                  .setColor(ColorF32{1.0f, 0.9f, 0.3f})
            ).push(Immediate2D::Path({
                       {400.0f, 500.0f}, {550.0f, 500.0f}, {600.0f, 600.0f}, {750.0f, 600.0f}, {850.0f, 550.0f},
                       {900.0f, 700.0f}, {1100.0f, 710.0f}, {1150.0f, 500.0f}
                   })
                   .setThickness(50.0f)
                   .setColor(ColorF32{0.3f, 1.0f, 0.7f})
            )
            .push(Immediate2D::Text(m_zxProtoBitmap, U"強化人間-san IS VERY INTERESTING")
                  .setPosition({300, 300})
                  .setColor(ColorF32{0.7, 0.4, 1.0})
            )
            .push(Immediate2D::Path({{1000, 600}, {1200, 600}, {1300, 800}, {1200, 1000}, {1000, 1000}, {900, 800}})
                  .setThickness(50.0f)
                  .setColor(ColorF32{0.1f, 1.0f, 0.3f})
                  .asCycle()
                // )
                // .push(Immediate2D::Text(m_rocknRollOneSdf, U"メイン")
                //       .setSize(200.0f)
                //       .setPosition(Scene::Center().movedBy(0, 100), Alignment9::MiddleCenter)
                //       .setColor(ColorF32{0.7, 1.0, 0.3})
            ).push(Immediate2D::Text(m_rocknRollOneBitmap, U"メインシステム: 戦闘モード起動")
                   .setSize(16.0f)
                   .setPosition(Screen::Center(), Alignment9::MiddleLeft)
                   .setColor(ColorF32{0.7})
            ).push(Immediate2D::Text(m_rocknRollOneBitmap, U"メインシステム")
                   .setSize(64.0f)
                   .setPosition(Screen::Center().movedBy(0, -100), Alignment9::MiddleCenter)
                   .setColor(ColorF32{0.7, 1.0, 0.3})
            );

        ImmediateDrawer::Global().draw();

        if (KeySpace.down())
        {
            ImmediateDrawer::Global() = ImmediateDrawer{};
        }

        ImmediateDrawer::Global()
            .push(Immediate2D::Rect{RectF{50, 500, 50, 50}})
            .draw();

        {
            const auto bind = m_miniMap.scopedBind();

            ImmediateDrawer::Global()
                .push(Immediate2D::Rect{RectF{64, 64, 128, 128}}.setColor(ColorF32{1.0f, 0.5f, 0.7f}));
            ImmediateDrawer::Global().draw();
        }

        m_miniMapDrawer.as2D().draw(Float2{500, 10});

        // -----------------------------------------------

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
                        m_cb.phongLight->lightDirection.x,
                        m_cb.phongLight->lightDirection.y,
                        m_cb.phongLight->lightDirection.z);

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

void Testbed_ImmediateDrawer()
{
    Testbed_ImmediateDrawer_impl impl{};

    Screen::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
