#include "pch.h"
#include "Demo_FSR4.h"

#include "imgui/imgui.h"

#include <dxgi1_6.h>

#include "TY/ConstantBufferWrapper.h"
#include "TY/Gamepad.h"
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
#include "TY/Scene.h"
#include "TY/PrimitiveModel3D.h"
#include "TY/SimpleCamera3D.h"
#include "TY/SimpleInput.h"

// #include "ffx_fsr2.h"
// #pragma comment(lib, "ffx_fsr2_api_x64.lib")
// #pragma comment(lib, "ffx_fsr2_api_dx12_x64.lib")

#include "ffx_api.h"
#pragma comment(lib, "amd_fidelityfx_loader_dx12.lib")
#pragma comment(lib, "amd_fidelityfx_upscaler_dx12.lib")

#include "TY/Logger.h"
#include "TY/detail/EngineRenderContext.h"

using namespace TY;

namespace
{
    struct Shadertoy_b10
    {
        Float2 g_screenResolution{};
        Float2 g_mousePosition{};
        Float2 g_jitterOffset{};
    };

    struct ToyModelBuffer : IGenericModelBuffer
    {
        GenericModelShapeBufferElement m_shape{};

        ToyModelBuffer()
        {
            m_shape.materialIndex = 0;
            m_shape.indexBuffer = IndexBuffer::Placeholder(6);
        }

        int shapeCount() const override
        {
            return 1; // Assuming a single shape
        }

        GenericModelShapeBufferElement shapeAt(int index) const override
        {
            return m_shape;
        }

        int materialCount() const override
        {
            return 1; // Assuming a single material for the shape
        }

        ConstantBufferCore materialCbv() const override
        {
            return ConstantBufferCore{1};
        }

        Array<Array<ShaderResourceType>> materialSrv() const override
        {
            return {};
        }
    };

    constexpr float fovFarZ = 1000.0f;

    constexpr float fovNearZ = 0.1f;
}

struct Demo_FSR4_impl
{
    struct
    {
        GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

        GraphicsShader shadertoy{GraphicsShader::VS_PS("asset/shader/shadertoy.hlsl")};
    } m_shaders;

    Mat4x4 m_projectionMat{};

    GenericModelDrawer m_toyDrawer{};

    ConstantBufferWrapper<Shadertoy_b10> m_toyCB{};

    Demo_FSR4_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_toyDrawer = GenericModelDrawerParams{}
                      .setModel(std::make_unique<ToyModelBuffer>())
                      .setVertexInput({})
                      .setShader(m_shaders.shadertoy)
                      .setOptions(GraphicsOptions{})
                      .setCbv10AndLater({m_toyCB});

        // InitFsr4();
    }

    // -----------------------------------------------

    RenderTarget m_inputRT{{.size = Scene::Size() * 0.5, .clearColor = ColorF32{0.0f, 1.0f}}};

    Float2 m_jitterOffset{};

    TextureDrawer m_upscaledOutputDrawer{};

    TextureDrawer m_debugInputRtDrawer{};

    // -----------------------------------------------

    void draw3D()
    {
        m_toyCB->g_screenResolution = Float2{m_inputRT.size()};
        m_toyCB->g_mousePosition = Mouse::PosF() * 0.5f;
        m_toyCB->g_jitterOffset = m_jitterOffset;
        m_toyCB.upload();

        m_toyDrawer.draw();
    }

    void Update()
    {
        static bool s_fsrEnabled{true};

        {
            auto bind = m_inputRT.scopedBind();
            draw3D();
        }

        if (s_fsrEnabled)
        {
            // DrawFsr4();
        }
        else
        {
            m_debugInputRtDrawer.as2D().resized(Scene::Size()).draw(Float2{});
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::Checkbox("FSR2 Enabled", &s_fsrEnabled);

            ImGui::End();
        }
    }

private:
};

void Demo_FSR4()
{
    Scene::RequestResize(Size{1920, 1080} * 2);
    System::Update();

    Demo_FSR4_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
