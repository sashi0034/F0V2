#include "pch.h"
#include "Testbed_Shadertoy.h"

#include "imgui/imgui.h"
#include "TY/ComputeDispatcher.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/Gamepad.h"
#include "TY/ImmediateDrawer.h"
#include "TY/InlineComponent.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Screen.h"
#include "TY/Shader.h"
#include "TY/System.h"

#define A_CPU
#include "asset/fsr1/ffx_a.h"
#include "asset/fsr1/ffx_fsr1.h"
#include "TY/KeyboardInput.h"
#include "TY/Utils.h"
#include "Util/ImmediatePrint.h"

using namespace TY;

namespace
{
    struct Shadertoy_b10
    {
        Float2 g_screenResolution{};
        Float2 g_mousePosition{};
        float g_time{};
    };

    struct EasuCB
    {
        std::array<uint32_t, 4> Const0{};
        std::array<uint32_t, 4> Const1{};
        std::array<uint32_t, 4> Const2{};
        std::array<uint32_t, 4> Const3{};
    };

    struct RcasCB
    {
        std::array<uint32_t, 4> Const0{};
    };

    struct Resource_Shadertoy : IInlineComponent
    {
        struct
        {
            ComputeShader shadertoy_cs{ShaderParams::CS("asset/shader/shadertoy_cs.hlsl")};

            ComputeShader fsr1_easu{ShaderParams::CS("asset/fsr1/fsr1_easu.hlsl")};

            ComputeShader fsr1_rcas{ShaderParams::CS("asset/fsr1/fsr1_rcas.hlsl")};
        } shader;

        struct
        {
            ConstantBufferWrapper<Shadertoy_b10> shadertoy{};
        } cb;
    };

    InlineComponent<Resource_Shadertoy> s_rsc{};

    class Fsr1Upscaler
    {
    public:
        void Init(const TextureHandle& input, const Size& outputSize)
        {
            m_inputSize = input.size();

            m_outputSize = outputSize;

            m_easuTexture =
                RenderTargetTextureParams()
                .setSize(outputSize)
                .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

            m_rcasTexture =
                RenderTargetTextureParams()
                .setSize(outputSize)
                .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

            m_easuDispatcher = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(s_rsc->shader.fsr1_easu)
                .setCbv({m_easuCB})
                .setSrv({input})
                .setUav({m_easuTexture})
            };

            m_rcasDispatcher = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(s_rsc->shader.fsr1_rcas)
                .setCbv({m_rcasCB})
                .setSrv({m_easuTexture})
                .setUav({m_rcasTexture})
            };
        }

        void Dispatch(float sharpnessAttenuation)
        {
            // EASU
            {
                EasuCB cb{};
                FsrEasuCon(reinterpret_cast<AU1*>(&cb.Const0),
                           reinterpret_cast<AU1*>(&cb.Const1),
                           reinterpret_cast<AU1*>(&cb.Const2),
                           reinterpret_cast<AU1*>(&cb.Const3),
                           static_cast<AF1>(m_inputSize.x),
                           static_cast<AF1>(m_inputSize.y),
                           static_cast<AF1>(m_inputSize.x),
                           static_cast<AF1>(m_inputSize.y),
                           static_cast<AF1>(m_outputSize.x),
                           static_cast<AF1>(m_outputSize.y));
                m_easuCB.uploadValue(cb);

                int groupsX = (Screen::Size().x + 15) / 16;
                int groupsY = (Screen::Size().y + 15) / 16;
                m_easuDispatcher.dispatch(groupsX, groupsY, 1);
            }

            // RCAS
            {
                RcasCB cb{};
                FsrRcasCon(reinterpret_cast<AU1*>(&cb.Const0), sharpnessAttenuation);
                m_rcasCB.uploadValue(cb);

                int groupsX = (Screen::Size().x + 15) / 16;
                int groupsY = (Screen::Size().y + 15) / 16;
                m_rcasDispatcher.dispatch(groupsX, groupsY, 1);
            }
        }

        TextureHandle OutputTexture() const
        {
            return m_rcasTexture;
        }

    private:
        Size m_inputSize{};
        Size m_outputSize{};

        UnorderedRenderTargetTexture m_easuTexture{};
        ConstantBufferWrapper<EasuCB> m_easuCB;
        ComputeDispatcher m_easuDispatcher{};

        UnorderedRenderTargetTexture m_rcasTexture{};
        ConstantBufferWrapper<RcasCB> m_rcasCB;
        ComputeDispatcher m_rcasDispatcher{};
    };
}

struct Testbed_Shadertoy_impl
{
    UnorderedRenderTargetTexture m_lowResolution{};
    ComputeDispatcher m_lowResolutionDispatcher{};

    UnorderedRenderTargetTexture m_nativeResolution{};
    ComputeDispatcher m_nativeResolutionDispatcher{};

    // -----------------------------------------------

    Fsr1Upscaler m_fsr1Upscaler{};

    Testbed_Shadertoy_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_lowResolution =
            RenderTargetTextureParams()
            .setSize(Screen::Size() * 0.5)
            .setClearColor(ColorF32{0.0f, 1.0f});

        m_nativeResolution =
            RenderTargetTextureParams()
            .setSize(Screen::Size())
            .setClearColor(ColorF32{0.0f, 1.0f});

        m_lowResolutionDispatcher =
            ComputeDispatcherParams{}
            .setCS(s_rsc->shader.shadertoy_cs)
            .setCbv({s_rsc->cb.shadertoy})
            .setUav({m_lowResolution});

        m_nativeResolutionDispatcher =
            ComputeDispatcherParams{}
            .setCS(s_rsc->shader.shadertoy_cs)
            .setCbv({s_rsc->cb.shadertoy})
            .setUav({m_nativeResolution});

        m_fsr1Upscaler.Init(m_lowResolution, Screen::Size());
    }

    // -----------------------------------------------

    void draw3D(const TextureHandle& texture, const ComputeDispatcher& dispatcher)
    {
        const Float2 textureSize = texture.size().cast<Float2>();
        s_rsc->cb.shadertoy->g_screenResolution = textureSize;
        s_rsc->cb.shadertoy->g_mousePosition = Mouse::PosF() * (textureSize / Screen::Size().cast<Float2>());
        s_rsc->cb.shadertoy->g_time += System::DeltaTime();
        s_rsc->cb.shadertoy.upload();

        const Size threadGroup = (texture.size() + Size{7, 7}) / 8;
        dispatcher.dispatch(threadGroup.x, threadGroup.y);
    }

    void Update()
    {
        static bool s_nativeResolution = false;
        static bool s_fsr1Enabled = true;
        static float s_sharpnessAttenuation = 0.0f;

        if (s_nativeResolution)
        {
            draw3D(m_nativeResolution, m_nativeResolutionDispatcher);
            Immediate2D::Texture(m_nativeResolution).resized(Screen::Size()).pushAuto();
        }
        else
        {
            draw3D(m_lowResolution, m_lowResolutionDispatcher);

            if (s_fsr1Enabled)
            {
                m_fsr1Upscaler.Dispatch(s_sharpnessAttenuation);
                Immediate2D::Texture(m_fsr1Upscaler.OutputTexture()).resized(Screen::Size()).pushAuto();
            }
            else
            {
                Immediate2D::Texture(m_lowResolution).resized(Screen::Size()).pushAuto();
            }
        }

        ImmediateDrawer::Global().draw();

        static bool s_ui{true};
        if (KeySpace.down())
        {
            s_ui = not s_ui;
        }

        if (s_ui)
        {
            if (std::string err = s_rsc->shader.shadertoy_cs.getErrorMessage(); not err.empty())
            {
                const auto messages = SplitStringView(err, '\n', true);
                for (const auto& s : messages)
                {
                    ImmediatePrint(s);
                }
            }

            ImGui::Begin("System Settings");

            ImGui::Text("Press Space to Toggle UI");

            ImGui::Separator();

            ImGui::Checkbox("Native Resolution", &s_nativeResolution);

            {
                ImGui::BeginDisabled(s_nativeResolution);

                ImGui::Checkbox("FSR1 Enabled", &s_fsr1Enabled);

                ImGui::SliderFloat("Sharpness Attenuation", &s_sharpnessAttenuation, 0.0f, 4.0f);

                ImGui::EndDisabled();
            }

            ImGui::Separator();

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
};

void Testbed_Shadertoy()
{
    Testbed_Shadertoy_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
