#include "pch.h"
#include "Title_Rendering.h"

#include "LivePPAddon.h"
#include "ZG/Buffer3D.h"
#include "ZG/Graphics3D.h"
#include "ZG/Image.h"
#include "ZG/KeyboardInput.h"
#include "ZG/Mat4x4.h"

#include "ZG/Shader.h"
#include "ZG/System.h"
#include "ZG/Texture.h"

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

    void Update_impl(
        Mat4x4& worldMat,
        DirectionLight_cb2& directionLight,
        ConstantBuffer<DirectionLight_cb2>& directionLightBuffer,
        const Model& model)
    {
        worldMat = worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));
        const Transformer3D t3d{worldMat};

        const float invRoot3 = std::sqrt(3.0);
        directionLight.lightDirection = Float3{invRoot3, -invRoot3, invRoot3};
        directionLight.lightColor = Float3{1.0f, 1.0f, 0.5f};
        directionLightBuffer.upload(directionLight);

        model.draw();
    }
}

void Title_Rendering()
{
    Mat4x4 worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

    const Mat4x4 viewMat = Mat4x4::LookAt(Vec3{0, 0, -5}, Vec3{0, 0, 0}, Vec3{0, 1, 0});

    const Mat4x4 projectionMat = Mat4x4::PerspectiveFov(
        90.0_deg,
        Scene::Size().horizontalAspectRatio(),
        1.0f,
        10.0f
    );

    const PixelShader modelPS{ShaderParams{.filename = "asset/shader/phong.hlsl", .entryPoint = "PS"}};
    const VertexShader modelVS{ShaderParams{.filename = "asset/shader/phong.hlsl", .entryPoint = "VS"}};

    DirectionLight_cb2 directionLight{};
    ConstantBuffer<DirectionLight_cb2> directionLightBuffer{1};

    const Model model{
        ModelParams{
            .filename = "asset/model/robot_head.obj", // "asset/model/cinnamon.obj"
            .ps = modelPS,
            .vs = modelVS,
            .cb2 = directionLightBuffer
        }
    };

    Graphics3D::SetViewMatrix(viewMat);
    Graphics3D::SetProjectionMatrix(projectionMat);

    // const Mat4x4 renderTargetProjectionMat = Mat4x4::PerspectiveFov(
    //     90.0_deg,
    //     renderTargetSize.horizontalAspectRatio(),
    //     1.0f,
    //     10.0f
    // );
    //
    // Graphics3D::SetProjectionMatrix(renderTargetProjectionMat);

    // worldMat = worldMat.translated(-5.0, 0.0, 0.0);;

    while (System::Update())
    {
#ifdef _DEBUG
        Util::AdvanceLivePP();
#endif

        {
            Update_impl(worldMat, directionLight, directionLightBuffer, model);
        }
    }
}
