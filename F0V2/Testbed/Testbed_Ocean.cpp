#include "pch.h"

#include "imgui/imgui.h"
#include "Testbed_Ocean.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
#include "TY/Gamepad.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBufferTemplates.h"
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

    struct DynamicOcean_cb
    {
        struct
        {
            int g_gridDensity;
            float g_gridSize;
            float g_time;
        };

        alignas(16) Float3 g_lightDirection;
        alignas(16) Float3 g_eyePosition;
    };

    struct OceanObject
    {
        Point gridPoint;
        GenericModelDrawer drawer;
    };
}

struct Testbed_Ocean_impl
{
    struct
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

        // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

        GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

        GraphicsShader blinn_phong{GraphicsShader::VS_PS("asset/shader/blinn_phong.hlsl")};

        GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};

        GraphicsShader dynamic_ocean{GraphicsShader::VS_PS("asset/shader/dynamic_ocean.hlsl")};
    } m_shaders;

    struct
    {
        ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};
    } m_models;

    struct
    {
        ConstantBufferWrapper<PhongLight_b4> phongLight{};
        ConstantBufferWrapper<DynamicOcean_cb> dynamicOcean{};
    } m_cb;

    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    ModelDrawer m_plainPlaneDrawer{};

    ModelDrawer m_playerDrawer{};
    Pose m_playerPose{};

    ModelDrawer m_mountainDrawer{};

    Array<OceanObject> m_oceanObjectsX128{};
    Array<OceanObject> m_oceanObjectsX64{};

    Testbed_Ocean_impl()
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

        m_plainPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::Plane(Float2{50.0f, 50.0f}, ColorU8{32, 200, 200, 255}.toColorF32()))
            .setShader(m_shaders.blinn_phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_playerDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.playerModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_playerPose.position.y = groundPositionY + 30.0f;

        m_mountainDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(m_models.mountainModel)
            .setShader(m_shaders.phong)
            .setCbv10AndLater({m_cb.phongLight})
        };

        m_oceanObjectsX128 = makeOceanObjects(128);
        m_oceanObjectsX64 = makeOceanObjects(64);
    }

    Array<OceanObject> makeOceanObjects(int density)
    {
        const int indexCount = (density - 1) * (density - 1) * 6;
        auto oceanModel = std::make_shared<SingleShapeModelBuffer>(indexCount, ConstantBufferImpl{1});

        Array<OceanObject> oceanObjects;
        constexpr int maxVisibleRange = 5;
        for (int x = -maxVisibleRange; x < maxVisibleRange; ++x)
        {
            for (int y = -maxVisibleRange; y < maxVisibleRange; ++y)
            {
                OceanObject obj{};
                obj.gridPoint = Point{x, y};

                obj.drawer = GenericModelDrawer{
                    GenericModelDrawerParams{}
                    .setModel(oceanModel)
                    .setVertexInput({})
                    .setShader(m_shaders.dynamic_ocean)
                    .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                    )
                    .setCbv10AndLater({m_cb.dynamicOcean})
                };

                oceanObjects.push_back(obj);
            }
        }

        return oceanObjects;
    }

    void Update()
    {
        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

        if (not KeyControl.pressed())
        {
            const float moveSpeed = KeyShift.pressed() ? 100.0f : 10.0f;
            m_camera.transformBySimpleInput(InGameDeltaTime(), moveSpeed);
        }
        else
        {
            const auto matrix = m_playerPose.getMatrix();
            const auto forward = matrix.forward();
            const auto right = matrix.right();
            const auto up = matrix.up();

            const auto moveInput = SimpleInput::GetPlayerMovement3D();
            constexpr auto speed = 10.0f;
            m_playerPose.position += forward * moveInput.z * speed * InGameDeltaTime();
            m_playerPose.position += right * moveInput.x * speed * InGameDeltaTime();
            m_playerPose.position += up * moveInput.y * speed * InGameDeltaTime();

            const auto rotateInput = SimpleInput::GetCameraRotation();
            constexpr auto rotationSpeed = 2.0f;
            m_playerPose.rotation *= Quaternion::RotateY(rotateInput.x * rotationSpeed * InGameDeltaTime());
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

        const auto lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();

        static struct
        {
            bool stop{};
            float gridSize{500.0f};
            bool subdivideX128{false};
            int visibleRange{1};
        } s_oceanSettings{};

        m_cb.dynamicOcean->g_gridDensity = s_oceanSettings.subdivideX128 ? 128 : 64;
        m_cb.dynamicOcean->g_gridSize = s_oceanSettings.gridSize;

        if (not s_oceanSettings.stop)
        {
            m_cb.dynamicOcean->g_time += InGameDeltaTime();
        }

        m_cb.dynamicOcean->g_eyePosition = m_camera.eyePosition();
        m_cb.dynamicOcean->g_lightDirection = lightDirection;
        m_cb.dynamicOcean.upload();

        m_cb.phongLight->lightDirection = lightDirection;
        m_cb.phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        m_cb.phongLight->eyePosition = m_camera.eyePosition();
        m_cb.phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};
        m_cb.phongLight.upload();

        m_planeLight->lightDirection = lightDirection;
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        m_plainPlaneDrawer.uploadWorldMatrix(Mat4x4::Translate({50.0f, -5.0f, 0.0f})).draw();

        // m_mountainDrawer.uploadWorldMatrix(Mat4x4::Scale(Float3{5.0})).draw();

        for (const auto& obj : s_oceanSettings.subdivideX128 ? m_oceanObjectsX128 : m_oceanObjectsX64)
        {
            if (Max(Abs(obj.gridPoint.x), Abs(obj.gridPoint.y)) > s_oceanSettings.visibleRange)
            {
                continue;
            }

            const auto worldMatrix = Mat4x4::Translate(Float3{
                obj.gridPoint.x * s_oceanSettings.gridSize,
                0,
                obj.gridPoint.y * s_oceanSettings.gridSize
            });

            obj.drawer.uploadWorldMatrix(worldMatrix).draw();
        }

        m_playerDrawer.draw();

        {
            ImGui::Begin("Ocean Settings");

            ImGui::Checkbox("Stop Ocean", &s_oceanSettings.stop);

            ImGui::Checkbox("Subdivide Ocean", &s_oceanSettings.subdivideX128);

            ImGui::InputFloat("Grid Size", &s_oceanSettings.gridSize, 1.0f, 10.0f, "%.2f");

            ImGui::InputInt("Visible Range", &s_oceanSettings.visibleRange, 1, 10);

            ImGui::End();
        }

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

void Testbed_Ocean()
{
    Testbed_Ocean_impl impl{};

    Screen::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
