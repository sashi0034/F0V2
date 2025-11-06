#include "pch.h"
#include "Testbed_RenderTarget.h"

#include "TY/DiskTexture.h"
#include "TY/DynamicTexture.h"
#include "TY/GpuMetrics.h"
#include "TY/Graphics3D.h"
#include "TY/Image.h"
#include "TY/KeyboardInput.h"
#include "TY/Mat4x4.h"

#include "TY/Shader.h"
#include "TY/System.h"
#include "TY/TextureDrawer.h"

#include "TY/Math.h"
#include "TY/ModelDrawer.h"
#include "TY/ModelLoader.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"
#include "TY/Transformer3D.h"

using namespace TY;

void Testbed_RenderTarget()
{
    const PixelShader default2dPS{ShaderParams{.filepath = "asset/shader/default2d.hlsl", .entryPoint = "PS"}};
    const VertexShader default2dVS{ShaderParams{.filepath = "asset/shader/default2d.hlsl", .entryPoint = "VS"}};

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

    const TextureDrawer noiseTexture{
        TextureDrawerParams{.texture = DynamicTexture(image), .shader = {default2dVS, default2dPS,}}
    };

    const TextureDrawer pngTexture{
        TextureDrawerParams{.texture = DiskTexture{"asset/image/mii.png"}, .shader = {default2dVS, default2dPS}}
    };

    Mat4x4 worldMat = Mat4x4::Identity().rotatedY(45.0_deg);

    const Mat4x4 viewMat = Mat4x4::LookAt(Float3{0, 0, -5}, Float3{0, 0, 0}, Float3{0, 1, 0});

    const Mat4x4 projectionMat = Mat4x4::PerspectiveFov(
        90.0_deg,
        Screen::Size().horizontalAspectRatio(),
        1.0f,
        10.0f
    );

    const PixelShader modelPS{ShaderParams{.filepath = "asset/shader/model.hlsl", .entryPoint = "PS"}};
    const VertexShader modelVS{ShaderParams{.filepath = "asset/shader/model.hlsl", .entryPoint = "VS"}};

    const ModelDrawer model{
        ModelDrawerParams{
            .model = ModelLoader::Load("asset/model/robot_head.obj"), // "asset/model/cinnamon.obj"
        }.setShader(modelVS, modelPS)
    };

    Graphics3D::SetViewMatrix(viewMat);
    Graphics3D::SetProjectionMatrix(projectionMat);

    constexpr Size renderTargetSize{640, 640};
    RenderTarget renderTarget{
        RenderTargetParams()
        .setRtvAndClearColor(RtvParams().setSize(renderTargetSize).setClearColor(ColorF32{1, 1, 0.5, 1}))
    };

    TextureDrawer renderTargetTexture{
        {
            .texture = renderTarget.asTexture(),
            .shader = {
                .vs = default2dVS,
                .ps = default2dPS,
            },
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
                pngTexture.as2D().drawAt(Screen::Center());
            }
            else
            {
                noiseTexture.as2D().drawAt(Screen::Center());
            }

            continue;
        }

        {
            const auto rt = renderTarget.scopedBind();

            worldMat = worldMat.rotatedY(Math::ToRadians(System::DeltaTime() * 90));
            model.uploadWorldMatrix(worldMat).draw();
        }

        constexpr Point someMargin = Point{64, 64};
        renderTargetTexture.as2D().draw(someMargin);

        {
            ImGui::Begin("System Settings");

            ImGui::Text("VRAM Usage: %.2f MB", GpuMetrics::MemoryUsage().estimateLocalUsageInMB());

            ImGui::End();
        }
    }
}
