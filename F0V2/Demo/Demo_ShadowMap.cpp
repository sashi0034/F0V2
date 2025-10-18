#include "pch.h"

#include "imgui/imgui.h"
#include "Demo_ShadowMap.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/DynamicTexture.h"
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
#include "TY/PrimitiveModel3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"

using namespace TY;

namespace
{
    constexpr float groundPositionY = -10.0f;

    constexpr float fovAngle = 75.0_deg;
    constexpr float fovNearZ = 0.1f;
    constexpr float fovFarZ = 1000.0f;

    constexpr GraphicsFormat shadowMapFormat = DXGI_FORMAT_R32_FLOAT;

    constexpr int cascadeShadowMapCount = 3;

    constexpr std::array<float, cascadeShadowMapCount> cascadeShadowMapSplits = {
        0.05 * fovFarZ, 0.25 * fovFarZ, fovFarZ
    };

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

    struct ShadowMap_cb
    {
        alignas(16) std::array<Mat4x4, cascadeShadowMapCount> worldToShadowProjection;
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

    struct CommonResource : IInlineComponent
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader model{GraphicsShader::VS_PS("asset/shader/model.hlsl")};

        // GraphicsShader lambert{GraphicsShader::VS_PS("asset/shader/lambert.hlsl")};

        GraphicsShader phong{GraphicsShader::VS_PS("asset/shader/phong.hlsl")};

        GraphicsShader phong_shadow{GraphicsShader::VS_PS("asset/shader/phong_shadow.hlsl")};

        GraphicsShader skydome{GraphicsShader::VS_PS("asset/shader/skydome.hlsl")};

        GraphicsShader shadowMapCaster{GraphicsShader::VS_PS("asset/shader/shadow_caster.hlsl")};

        GraphicsShader r32_float_visualizer{GraphicsShader::VS_PS("asset/shader/r32_float_visualizer.hlsl")};

        ModelBuffer playerModel{ModelLoader::Load("asset/model/tie_fighter.obj")};

        ModelBuffer mountainModel{ModelLoader::Load("asset/model/dirty_plane.obj")};

        ConstantBufferWrapper<PhongLight_b4> phongLight{};

        ConstantBufferWrapper<ShadowMap_cb> shadowMap_cb{};

        CommonResource()
        {
        }
    };

    InlineComponent<CommonResource> s_resource{};
}

struct Demo_ShadowMap_impl
{
    SimpleCamera3D m_camera{};

    ModelDrawer m_skydomeModel{};

    Mat4x4 m_projectionMat{};

    ConstantBufferWrapper<LambertLight_b4> m_planeLight{};

    ModelDrawer m_groundPlaneDrawer{};

    ModelDrawer m_playerDrawer{};
    ModelDrawer m_playerShadowDrawer{};
    ConstantBuffer<Mat4x4> m_playerShadowDrawerConstantBuffer{};
    Pose m_playerPose{};

    ModelDrawer m_mountainDrawer{};

    std::array<RenderTarget, cascadeShadowMapCount> m_shadowMaps{};

    std::array<TextureDrawer, cascadeShadowMapCount> m_shadowMapDebugDrawers{};

    Demo_ShadowMap_impl()
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
            .setShader(s_resource->skydome)
            .setOptions(GraphicsOptions::Default3D()
                        .setRasterizer(GraphicsRasterizerOptions::Default3D().setCull(GraphicsCullMode::None))
                        .setDepth(GraphicsDepthOptions::Default3D().setWriteMask(false))
            )
            .setCbv10AndLater({skydome_b4})
        };

        for (int i = 0; i < cascadeShadowMapCount; ++i)
        {
            m_shadowMaps[i] = RenderTarget{
                RenderTargetParams()
                .setRtvAndClearColor(RtvParams()
                                     .setSize(Size{2048, 2048})
                                     .setClearColor(ColorF32{1.0f, 1.0f})
                                     .setFormat(shadowMapFormat))
            };
        }

        for (int i = 0; i < cascadeShadowMapCount; ++i)
        {
            m_shadowMapDebugDrawers[i] = TextureDrawer{
                TextureDrawerParams{}
                .setTexture(m_shadowMaps[i].asTexture())
                .setShader(s_resource->r32_float_visualizer)
            };
        }

        const auto groundPlaneTexture = makeGroundPlane(
            Size{1024, 1024}, 32, ColorF32{0.9}, ColorF32{0.3});
        m_groundPlaneDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(PrimitiveModel3D::TexturePlane(groundPlaneTexture, Float2{1024.0f, 1024.0f}))
            .setShader(s_resource->model)
        }.uploadWorldMatrix(Mat4x4::Translate({0.0f, groundPositionY, 0.0f}));

        {
            m_playerDrawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(s_resource->playerModel)
                .setShader(s_resource->phong)
                .setCbv10AndLater({s_resource->phongLight})
            };

            // m_playerShadowDrawer = ModelDrawer{
            //     ModelDrawerParams{}
            //     .setModel(s_resource->playerModel)
            //     .setShaders(s_resource->shadowMapCaster)
            //     .setOptions(GraphicsOptions::Default3D().setRtvFormats({shadowMapFormat}))
            //     .setCB4(s_resource->shadowMap_cb)
            // };

            m_playerShadowDrawerConstantBuffer = ConstantBuffer<Mat4x4>{};
            m_playerShadowDrawer = ModelDrawer{
                ModelDrawerParams{}
                .setModel(s_resource->playerModel)
                .setShader(s_resource->shadowMapCaster)
                .setOptions(GraphicsOptions::Default3D().setRtvFormats({shadowMapFormat}))
                .setCbv10AndLater({m_playerShadowDrawerConstantBuffer})
            };
        }

        m_playerPose.position.y = groundPositionY + 15.0f;

        m_mountainDrawer = ModelDrawer{
            ModelDrawerParams{}
            .setModel(s_resource->mountainModel)
            .setShader(s_resource->phong_shadow)
            .setOptions(GraphicsOptions::Default3D().setSamplers({
                GraphicsSamplerOptions{}
                .setAddress(GraphicsAddressMode::Border)
                .setFilter(GraphicsFilterMode::Linear),
                GraphicsSamplerOptions{}
                .setFilter(GraphicsFilterMode::Linear)
                .setComparison(GraphicsComparisonFunction::Greater)
                .setMaxAnisotropy(1)
            }))
            .setCbv10AndLater({s_resource->phongLight, s_resource->shadowMap_cb})
            .setSrv10AndLater({
                m_shadowMaps[0].asTexture(),
                m_shadowMaps[1].asTexture(),
                m_shadowMaps[2].asTexture()
            })
        };
    }

    void Update()
    {
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
                fovAngle,
                Scene::Size().horizontalAspectRatio(),
                fovNearZ,
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

        m_playerDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();

        // 影の更新
        {
            updateCascadeShadowMapMatrix();

            for (int i = 0; i < m_shadowMaps.size(); ++i)
            {
                m_playerShadowDrawerConstantBuffer.upload(s_resource->shadowMap_cb->worldToShadowProjection[i]);

                const auto rt = m_shadowMaps[i].scopedBind();

                // 影の対象のオブジェクトを描画
                {
                    m_playerShadowDrawer.uploadWorldMatrix(m_playerPose.getMatrix()).draw();
                }
            }

            m_mountainDrawer.uploadWorldMatrix(Mat4x4::Scale(Float3{5.0})).draw();
        }

        for (int i = 0; i < m_shadowMaps.size(); ++i)
        {
            m_shadowMapDebugDrawers[i].as2D().resized({196.0f, 196.0f}).draw({200.0f * i, 0.0});
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

    void updateCascadeShadowMapMatrix()
    {
        const Float3 cameraForward = m_camera.worldMatrix().forward();
        const Float3 cameraRight = m_camera.worldMatrix().right();
        const Float3 cameraUp = m_camera.worldMatrix().up();

        const Float3 cameraEye = m_camera.eyePosition();

        // -----------------------------------------------

        const auto shadowEyePosition = cameraEye - s_resource->phongLight->lightDirection * (fovFarZ * 0.5f);

        const auto shadowProjection = Mat4x4::PerspectiveFov(
            45.0_deg,
            1.0f,
            fovNearZ,
            fovFarZ
        );

        const auto shadowView = Mat4x4::LookAt(
            shadowEyePosition,
            cameraEye,
            Float3{0.0f, 1.0f, 0.0f}
        );

        const auto shadowViewProjection = shadowView * shadowProjection;

        // -----------------------------------------------

        float nearDepth = fovNearZ;
        for (int i = 0; i < cascadeShadowMapCount; ++i)
        {
            //                 /|                ---
            //                / |                 |
            //               /  |                 |
            //              /   |                 | farHalfH 
            //             /|   |  ---            |
            //            / |   |   | nearHalfH   |
            // cameraEye |--|---|  ---           ---
            //            \ |   |
            //             \|   |
            //              \   |
            //               \  |
            //                \ |
            //                 \|

            const float farDepth = cascadeShadowMapSplits[i];

            const float nearHalfH = tanf(fovAngle / 2.0f) * nearDepth;
            const float nearHalfW = nearHalfH * Scene::Size().horizontalAspectRatio();

            const float farHalfH = tanf(fovAngle / 2.0f) * farDepth;
            const float farHalfW = farHalfH * Scene::Size().horizontalAspectRatio();

            const Float3 nearCenter = cameraEye + cameraForward * nearDepth;
            const Float3 farCenter = cameraEye + cameraForward * farDepth;

            std::array<Float3, 8> frustumCorners{};
            frustumCorners[0] = nearCenter + cameraRight * nearHalfW - cameraUp * nearHalfH;
            frustumCorners[1] = nearCenter + cameraRight * nearHalfW + cameraUp * nearHalfH;
            frustumCorners[2] = nearCenter - cameraRight * nearHalfW + cameraUp * nearHalfH;
            frustumCorners[3] = nearCenter - cameraRight * nearHalfW - cameraUp * nearHalfH;
            frustumCorners[4] = farCenter + cameraRight * farHalfW - cameraUp * farHalfH;
            frustumCorners[5] = farCenter + cameraRight * farHalfW + cameraUp * farHalfH;
            frustumCorners[6] = farCenter - cameraRight * farHalfW + cameraUp * farHalfH;
            frustumCorners[7] = farCenter - cameraRight * farHalfW - cameraUp * farHalfH;

            Float3 minP{FLT_MAX, FLT_MAX, FLT_MAX};
            Float3 maxP{-FLT_MAX, -FLT_MAX, -FLT_MAX};
            for (auto& corner : frustumCorners)
            {
                corner = shadowViewProjection.transformPoint(corner);
                minP = MinVector3(minP, corner);
                maxP = MaxVector3(maxP, corner);
            }

            // クロップ行列を作成
            Float2 scaling = Float2{2.0f, 2.0f} / (maxP.xy() - minP.xy());
            Float2 translation = -(minP.xy() + maxP.xy()) * Float2{0.5f, 0.5f} * scaling;
            Mat4x4 cropMatrix = Mat4x4::Identity();
            cropMatrix.mat.r[0].m128_f32[0] = scaling.x;
            cropMatrix.mat.r[1].m128_f32[1] = scaling.y;
            cropMatrix.mat.r[3].m128_f32[0] = translation.x;
            cropMatrix.mat.r[3].m128_f32[1] = translation.y;

            s_resource->shadowMap_cb->worldToShadowProjection[i] = shadowViewProjection * cropMatrix;

            nearDepth = farDepth; // 次の分割のために更新
        }

        s_resource->shadowMap_cb.upload();
    }
};

void Demo_ShadowMap()
{
    Demo_ShadowMap_impl impl{};

    Scene::RequestResize({1920, 1080});

    while (System::Update())
    {
        impl.Update();
    }
}
