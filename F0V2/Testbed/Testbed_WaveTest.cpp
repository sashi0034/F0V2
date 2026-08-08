#include "pch.h"
#include "Testbed_WaveTest.h"

#include "imgui/imgui.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/InlineComponent.h"
#include "TY/KeyboardInput.h"

#include "TY/Shader.h"
#include "TY/System.h"

#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"
#include "TY/Window.h"

using namespace TY;

namespace
{
    struct WaveTest_b10
    {
        Float2 g_screenResolution{};
        Float2 g_mousePosition{};
    };

    struct Resource_Testbed_WaveTest : IInlineComponent
    {
        struct
        {
            GraphicsShader wave_test{GraphicsShader::VS_PS("asset/shader/wave_test.hlsl")};
        } shader;
    };

    InlineComponent<Resource_Testbed_WaveTest> s_rsc{};
}

struct Testbed_WaveTest_impl
{
    GenericModelDrawer m_spriteDrawer{};

    ConstantBufferWrapper<WaveTest_b10> m_cb10{};

    Testbed_WaveTest_impl()
    {
        m_spriteDrawer =
            GenericModelDrawerParams{}
            .setModel(std::make_unique<SingleShapeModelBuffer>(6))
            .setVertexInput({})
            .setShader(s_rsc->shader.wave_test)
            .setOptions(GraphicsOptions{})
            .setCbv10AndLater({m_cb10});
    }

    void Update()
    {
        {
            const Float2 renderTargetSize = Float2{RenderTarget::Current().size()};
            m_cb10->g_screenResolution = renderTargetSize;
            m_cb10->g_mousePosition = Mouse::PosF() * (renderTargetSize / Screen::Size().cast<Float2>());
            m_cb10.upload();

            m_spriteDrawer.draw();
        }

        static bool s_showUI{};
        if (KeySpace.down())
        {
            s_showUI = not s_showUI;
        }

        if (not s_showUI)
        {
            {
                ImGui::Begin("Camera");

                ImGui::Separator();

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
    }
};

void Testbed_WaveTest()
{
    Testbed_WaveTest_impl impl{};

    // Screen::RequestResize(Size{1920, 1080});
    // Window::Resize(Size{1920, 1080});

    Screen::RequestResize(Size{800, 450});
    Window::Resize(Size{800, 450});

    while (System::Update())
    {
        impl.Update();
    }
}
