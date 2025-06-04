#include "pch.h"
#include "Title_Phong.h"

#include "LivePPAddon.h"
#include "ZG/Graphics3D.h"
#include "ZG/KeyboardInput.h"
#include "ZG/Mat4x4.h"

#include "ZG/Shader.h"
#include "ZG/System.h"

#include "ZG/Math.h"
#include "ZG/Model.h"
#include "ZG/RenderTarget.h"
#include "ZG/Scene.h"
#include "ZG/Transformer3D.h"

using namespace ZG;

namespace
{
    struct DirectionLight_cb2
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
    };

    const std::string shader_lambert = "asset/shader/lambert.hlsl";
}

struct Title_PointLight_impl
{
    Mat4x4 m_worldMat{};

    Mat4x4 m_viewMat{};

    Mat4x4 m_projectionMat{};

    PixelShader m_modelPS{};
    VertexShader m_modelVS{};

    DirectionLight_cb2 m_planeLight{};
    ConstantBuffer<DirectionLight_cb2> m_planeLightBuffer{1};

    DirectionLight_cb2 m_directionLight{};
    ConstantBuffer<DirectionLight_cb2> m_directionLightBuffer{1};

    Model m_planeModel{};
    Model m_robotModel{};

    Title_PointLight_impl()
    {
        m_worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

        m_viewMat = Mat4x4::LookAt(Vec3{0, 0, -5}, Vec3{0, 0, 0}, Vec3{0, 1, 0});

        m_projectionMat = Mat4x4::PerspectiveFov(
            90.0_deg,
            Scene::Size().horizontalAspectRatio(),
            1.0f,
            10.0f
        );

        m_modelPS = PixelShader{ShaderParams{.filename = shader_lambert, .entryPoint = "PS"}};
        m_modelVS = VertexShader{ShaderParams{.filename = shader_lambert, .entryPoint = "VS"}};

        m_planeModel = Model{
            ModelParams{
                .filename = "asset/model/dirty_plane.obj",
                .ps = m_modelPS,
                .vs = m_modelVS,
                .cb2 = m_planeLightBuffer
            }
        };

        m_robotModel = Model{
            ModelParams{
                .filename = "asset/model/robot_head.obj", // "asset/model/cinnamon.obj"
                .ps = m_modelPS,
                .vs = m_modelVS,
                .cb2 = m_directionLightBuffer
            }
        };
    }

    void Update()
    {
        updateCamera();

        m_worldMat = m_worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));

        {
            m_planeLight.lightDirection = Float3(0.5f, -1.0f, 0.5f).normalized();
            m_planeLight.lightColor = Float3{1.0f, 1.0f, 0.5f};
            m_planeLightBuffer.upload(m_planeLight);

            m_planeModel.draw();
        }

        {
            const Transformer3D t3d{m_worldMat};

            m_directionLight.lightDirection = m_worldMat.forward().normalized();
            m_directionLight.lightColor = Float3{1.0f, 1.0f, 0.5f};
            m_directionLightBuffer.upload(m_directionLight);

            m_robotModel.draw();
        }
    }

    void updateCamera()
    {
        constexpr float moveSpeed = 10.0f;
        constexpr float rotationSpeed = 50.0f;

        Float3 moveVector{};
        moveVector.x = (KeyD.pressed() ? 1.0f : 0.0f) - (KeyA.pressed() ? 1.0f : 0.0f);
        moveVector.y = (KeyX.pressed() ? 1.0f : 0.0f) - (KeyE.pressed() ? 1.0f : 0.0f);
        moveVector.z = (KeyS.pressed() ? 1.0f : 0.0f) - (KeyW.pressed() ? 1.0f : 0.0f);
        if (not moveVector.isZero())
        {
            m_viewMat = m_viewMat.translated(moveVector * moveSpeed * System::DeltaTime());

            std::cout << "Camera position: "
                << m_viewMat.translation().x << ", "
                << m_viewMat.translation().y << ", "
                << m_viewMat.translation().z << std::endl;
        }

        Float2 rotateVector{};
        rotateVector.x = (KeyRight.pressed() ? 1.0f : 0.0f) - (KeyLeft.pressed() ? 1.0f : 0.0f);
        rotateVector.y = (KeyDown.pressed() ? 1.0f : 0.0f) - (KeyUp.pressed() ? 1.0f : 0.0f);
        if (not rotateVector.isZero())
        {
            m_viewMat = m_viewMat
                        .rotatedY(Math::ToRadians(rotateVector.x * rotationSpeed * System::DeltaTime()))
                        .rotatedX(Math::ToRadians(rotateVector.y * rotationSpeed * System::DeltaTime()));

            const auto rotation = m_viewMat.eulerRotation();
            std::cout << "Camera rotation: "
                << rotation.x << ", "
                << rotation.y << ", "
                << rotation.z << std::endl;
        }

        Graphics3D::SetViewMatrix(m_viewMat);
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
