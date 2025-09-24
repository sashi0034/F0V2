#include "pch.h"
#include "Demo_Collision2.h"

#include "imgui/imgui.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/Gamepad.h"
#include "TY/Graphics3D.h"
#include "TY/InlineComponent.h"
#include "TY/Intersects3D.h"
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

    // -----------------------------------------------

    struct Resource_Demo_Collision2 : IInlineComponent
    {
        struct
        {
            GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

            GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

            // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

            GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

            GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};
        } shaders;

        struct models_t
        {
            ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

            ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};
        } models;

        struct
        {
            ConstantBufferWrapper<PhongLight_b4> phongLight{};
        } cb;
    };

    InlineComponent<Resource_Demo_Collision2> s_resource_Demo_Collision2{};

    Resource_Demo_Collision2& getRsc()
    {
        return s_resource_Demo_Collision2.get();
    }

    // -----------------------------------------------

    struct TriangleObject
    {
        Triangle3D m_tri{
            Float3{-10, 1, 5},
            Float3{10, 1, 5},
            Float3{0, 10, 5}
        };

        ModelBuffer m_model;
        ModelDrawer m_drawer;

        void Init()
        {
            m_model = ModelBuffer{PrimitiveModel3D::Triangle(m_tri, ColorF32{1.0f, 0.7f, 0.5f})};

            m_drawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(m_model)
                .setShader(getRsc().shaders.phong)
                .setCbv10AndLater({getRsc().cb.phongLight})
            };
        }

        void DebugUI()
        {
            ImGui::Begin("Triangle");

            bool changed = false;
            changed |= ImGui::DragFloat3("v0", &m_tri.p0.x, 0.1f);
            changed |= ImGui::DragFloat3("v1", &m_tri.p1.x, 0.1f);
            changed |= ImGui::DragFloat3("v2", &m_tri.p2.x, 0.1f);

            if (changed)
            {
                Init();
            }

            ImGui::End();
        }
    };

    struct CapsuleObject
    {
        float m_radius = 1;
        float m_height = 2;

        ModelBuffer m_model;
        ModelDrawer m_drawer;
        Float3 m_pos{};

        void Init()
        {
            m_model = ModelBuffer{PrimitiveModel3D::Capsule(m_radius, m_height, ColorF32{0.5f, 0.7f, 1.0f})};

            m_drawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(m_model)
                .setShader(getRsc().shaders.phong)
                .setCbv10AndLater({getRsc().cb.phongLight})
            };
        }

        void DebugUI()
        {
            ImGui::Begin("Capsule");

            bool changed = false;
            changed |= ImGui::DragFloat3("Pos", &m_pos.x, 0.05f);

            changed |= ImGui::DragFloat("Radius", &m_radius, 0.1f, 0.1f, 100.0f);
            changed |= ImGui::DragFloat("Height", &m_height, 0.1f, 0.1f, 100.0f);
            if (changed)
            {
                Init();
            }

            ImGui::End();
        }
    };
}

struct Demo_Collision2_impl
{
    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    TriangleObject m_triangleObject{};
    CapsuleObject m_capsuleObject{};

    Demo_Collision2_impl()
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
            .setShader(getRsc().shaders.skydome)
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
            .setShader(getRsc().shaders.model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        m_triangleObject.Init();
        m_capsuleObject.Init();
    }

    void Update()
    {
        m_camera.transformBySimpleInput();

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

        getRsc().cb.phongLight->lightDirection = Float3{0.3f, -1.0f, 0.3f}.normalized();
        getRsc().cb.phongLight->lightColor = Float3{1.0f, 1.0f, 0.5f};
        getRsc().cb.phongLight->eyePosition = m_camera.eyePosition();
        getRsc().cb.phongLight->ambientColor = Float3{0.3f, 0.35f, 0.35f};

        getRsc().cb.phongLight.upload();

        m_planeLight->lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
        m_planeLight->lightColor = Float3{1.0f, 1.0f, 1.0f};
        m_planeLight.upload();

        // -----------------------------------------------

        m_skydomeModel.uploadWorldMatrix(Mat4x4::Translate(m_camera.eyePosition())).draw();

        m_groundPlaneDrawer.draw();

        // -----------------------------------------------

        static bool s_moveCapsuleWithCamera = true;

        {
            const Float3 previousPos = m_capsuleObject.m_pos;
            Float3 newPos = previousPos;

            if (s_moveCapsuleWithCamera)
            {
                newPos = m_camera.eyePosition() + m_camera.worldMatrix().forward() * 10.0f;
            }

            if (previousPos != newPos)
            {
                const Float3 bottomOffset = Float3{0, -m_capsuleObject.m_height * 0.5f, 0};
                const Float3 topOffset = -bottomOffset;

                const auto moveTestCapsuleT =
                    Capsule{previousPos + bottomOffset, newPos + bottomOffset, m_capsuleObject.m_radius};
                const auto moveTestCapsule =
                    Capsule{previousPos, newPos, m_capsuleObject.m_radius};
                const auto moveTestCapsuleB =
                    Capsule{previousPos + topOffset, newPos + topOffset, m_capsuleObject.m_radius};

                if (Intersects(moveTestCapsule, m_triangleObject.m_tri) ||
                    Intersects(moveTestCapsuleT, m_triangleObject.m_tri) ||
                    Intersects(moveTestCapsuleB, m_triangleObject.m_tri))
                {
                    const auto plane = m_triangleObject.m_tri.asPlane();
                    const float distance = m_triangleObject.m_tri.asPlane().signedDistanceFrom(previousPos);
                    const float moveAmound = distance + ((m_capsuleObject.m_radius + 1e-2) * (distance > 0 ? -1 : 1));
                    const Float3 normal = -plane.normal;
                    m_capsuleObject.m_pos = previousPos + normal * moveAmound;
                }
                else
                {
                    m_capsuleObject.m_pos = newPos;
                }
            }
        }
        m_capsuleObject.m_drawer.uploadWorldMatrix(Mat4x4::Translate(m_capsuleObject.m_pos)).draw();

        m_capsuleObject.DebugUI();

        Capsule testCapsule = Capsule::AlongY(
            m_capsuleObject.m_pos, m_capsuleObject.m_height, m_capsuleObject.m_radius);

        m_triangleObject.m_drawer.draw();
        m_triangleObject.DebugUI();

        bool intersectionTest = Intersects(testCapsule, m_triangleObject.m_tri);

        {
            ImGui::Begin("Intersection Test");

            if (intersectionTest)
            {
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Intersection: True");
            }
            else
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Intersection: False");
            }

            ImGui::Checkbox("Move Capsule with Camera", &s_moveCapsuleWithCamera);
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
                        getRsc().cb.phongLight->lightDirection.x,
                        getRsc().cb.phongLight->lightDirection.y,
                        getRsc().cb.phongLight->lightDirection.z);

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

void Demo_Collision2()
{
    Demo_Collision2_impl impl{};

    Scene::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
