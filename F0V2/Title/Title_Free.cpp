#include "pch.h"
#include "Title_Free.h"

#include "TY/Buffer3D.h"
#include "TY/Graphics3D.h"
#include "TY/Image.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"
#include "TY/Texture.h"

#include "TY/Math.h"
#include "TY/Model.h"
#include "TY/ModelLoader.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Transformer3D.h"

using namespace TY;

void Title_Free()
{
    const PixelShader default2dPS{ShaderParams{.filename = "asset/shader/default2d.hlsl", .entryPoint = "PS"}};
    const VertexShader default2dVS{ShaderParams{.filename = "asset/shader/default2d.hlsl", .entryPoint = "VS"}};

    Image image{Size{16, 16}};
    for (int x = 0; x < image.size().x; ++x)
    {
        for (int y = 0; y < image.size().y; ++y)
        {
            auto& pixel = image[Point{x, y}];
            pixel.r = rand() % 256;
            pixel.g = rand() % 256;
            pixel.b = rand() % 256;
            pixel.a = 255;
        }
    }

    const Texture noiseTexture{
        TextureParams{.source = image, .ps = default2dPS, .vs = default2dVS}
    };

    const Texture pngTexture{
        TextureParams{.source = "asset/image/mii.png", .ps = default2dPS, .vs = default2dVS}
    };

    Mat4x4 worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

    const Mat4x4 viewMat = Mat4x4::LookAt(Vec3{0, 0, -5}, Vec3{0, 0, 0}, Vec3{0, 1, 0});

    const Mat4x4 projectionMat = Mat4x4::PerspectiveFov(
        90.0_deg,
        Scene::Size().horizontalAspectRatio(),
        1.0f,
        10.0f
    );

    const PixelShader modelPS{ShaderParams{.filename = "asset/shader/model_pixel.hlsl", .entryPoint = "PS"}};
    const VertexShader modelVS{ShaderParams{.filename = "asset/shader/model_vertex.hlsl", .entryPoint = "VS"}};

    const Model model{
        ModelParams{
            .data = ModelLoader::Load("asset/model/robot_head.obj"), // "asset/model/cinnamon.obj"
            .ps = modelPS,
            .vs = modelVS,
        }
    };

    Graphics3D::SetViewMatrix(viewMat);
    Graphics3D::SetProjectionMatrix(projectionMat);

    constexpr Size renderTargetSize{640, 640};
    RenderTarget renderTarget{
        {
            .size = renderTargetSize,
            .clearColor = ColorF32{1, 1, 0.5, 1},
        }
    };

    Texture renderTargetTexture{
        {
            .source = renderTarget.getResource(),
            .ps = default2dPS,
            .vs = default2dVS
        }
    };

    // const Mat4x4 renderTargetProjectionMat = Mat4x4::PerspectiveFov(
    //     90.0_deg,
    //     renderTargetSize.horizontalAspectRatio(),
    //     1.0f,
    //     10.0f
    // );
    //
    // Graphics3D::SetProjectionMatrix(renderTargetProjectionMat);

    // worldMat = worldMat.translated(-5.0, 0.0, 0.0);;

    int count{};
    while (System::Update())
    {
        if (KeySpace.pressed())
        {
            count++;
            if (count % 120 < 60)
            {
                pngTexture.drawAt(Scene::Center());
            }
            else
            {
                noiseTexture.drawAt(Scene::Center());
            }

            continue;
        }

        {
            const auto rt = renderTarget.scopedBind();

            worldMat = worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));
            const Transformer3D t3d{worldMat};

            model.draw();
        }

        constexpr Point someMargin = Point{64, 64};
        renderTargetTexture.draw(RectF{someMargin, renderTarget.size()});
    }
}
