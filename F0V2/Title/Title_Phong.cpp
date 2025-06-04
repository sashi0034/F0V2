#include "pch.h"
#include "Title_Phong.h"

#include "LivePPAddon.h"
#include "ZG/Graphics3D.h"
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
    struct PhongLight_cb2
    {
        alignas(16) Float3 lightDirection;
        alignas(16) Float3 lightColor{};
        alignas(16) Float3 eyePosition{};
        alignas(16) Float3 ambientColor{};
    };

    const std::string shaderPath = "asset/shader/phong.hlsl";

    constexpr Vec3 eyePosition{0.0, 0.0, -5.0};
}

struct App
{
    Mat4x4 m_worldMat{};

    Mat4x4 m_viewMat{};

    Mat4x4 m_projectionMat{};

    PixelShader m_modelPS{};
    VertexShader m_modelVS{};

    PhongLight_cb2 m_directionLight{};
    ConstantBuffer<PhongLight_cb2> m_directionLightBuffer{1};

    Model m_model{};

    App()
    {
        m_worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

        m_viewMat = Mat4x4::LookAt(Vec3{0, 0, -5}, Vec3{0, 0, 0}, Vec3{0, 1, 0});

        m_projectionMat = Mat4x4::PerspectiveFov(
            90.0_deg,
            Scene::Size().horizontalAspectRatio(),
            1.0f,
            10.0f
        );

        m_modelPS = PixelShader{ShaderParams{.filename = shaderPath, .entryPoint = "PS"}};
        m_modelVS = VertexShader{ShaderParams{.filename = shaderPath, .entryPoint = "VS"}};

        m_model = Model{
            ModelParams{
                .filename = "asset/model/robot_head.obj",
                .ps = m_modelPS,
                .vs = m_modelVS,
                .cb2 = m_directionLightBuffer
            }
        };

        Graphics3D::SetViewMatrix(m_viewMat);
        Graphics3D::SetProjectionMatrix(m_projectionMat);
    }

    void Update()
    {
        m_worldMat = m_worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));
        const Transformer3D t3d{m_worldMat};

        m_directionLight.lightDirection = m_worldMat.forward().normalized();
        m_directionLight.lightColor = Float3{1.0f, 1.0f, 0.5f};

        m_directionLight.eyePosition = eyePosition;

        m_directionLight.ambientColor = Float3{0.1f};

        m_directionLightBuffer.upload(m_directionLight);

        m_model.draw();
    }
};

void Title_Phong()
{
    App impl{};

    while (System::Update())
    {
#ifdef _DEBUG
        Util::AdvanceLivePP();
#endif

        impl.Update();
    }
}
