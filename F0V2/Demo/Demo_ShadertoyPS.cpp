#include "pch.h"
#include "Demo_ShadertoyPS.h"

#include "imgui/imgui.h"

#include "TY/ConstantBufferWrapper.h"
#include "TY/Gamepad.h"
#include "TY/InlineComponent.h"
#include "TY/ModelDrawer.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Scene.h"
#include "TY/Shader.h"
#include "TY/System.h"

using namespace TY;

namespace
{
    struct Shadertoy_b10
    {
        Float2 g_screenResolution{};
        Float2 g_mousePosition{};
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

    struct Resource_Shadertoy : IInlineComponent
    {
        struct
        {
            GraphicsShader default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

            GraphicsShader shadertoy{GraphicsShader::VS_PS("asset/shader/shadertoy_ps.hlsl")};
        } shader;

        struct
        {
            ConstantBufferWrapper<Shadertoy_b10> shadertoy{};
        } cb;
    };

    InlineComponent<Resource_Shadertoy> s_rsc{};
}

struct Demo_ShadertoyPS_impl
{
    GenericModelDrawer m_toyDrawer{};

    TextureDrawer m_upscaledOutputDrawer{};

    RenderTarget m_lowResolution{};

    TextureDrawer m_lowResolutionDrawer{};

    Demo_ShadertoyPS_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_toyDrawer =
            GenericModelDrawerParams{}
            .setModel(std::make_unique<ToyModelBuffer>())
            .setVertexInput({})
            .setShader(s_rsc->shader.shadertoy)
            .setOptions(GraphicsOptions{})
            .setCbv10AndLater({s_rsc->cb.shadertoy});

        m_lowResolution =
            RenderTargetParams()
            .setSize(Scene::Size() * 0.5)
            .setClearColor(ColorF32{0.0f, 1.0f});

        m_lowResolutionDrawer =
            TextureDrawerParams{}
            .setTexture(m_lowResolution.asShaderResource())
            .setShader(s_rsc->shader.default2d);
    }

    // -----------------------------------------------

    void draw3D()
    {
        const Float2 renderTargetSize = Float2{RenderTarget::Current().size()};
        s_rsc->cb.shadertoy->g_screenResolution = renderTargetSize;
        s_rsc->cb.shadertoy->g_mousePosition = Mouse::PosF() * (renderTargetSize / Scene::Size().cast<Float2>());
        s_rsc->cb.shadertoy.upload();

        m_toyDrawer.draw();
    }

    void Update()
    {
        static bool s_upscalingEnabled{true};

        if (s_upscalingEnabled)
        {
            {
                auto bind = m_lowResolution.scopedBind();
                draw3D();
            }

            m_lowResolutionDrawer.as2D().resized(Scene::Size()).draw(Float2{});
        }
        else
        {
            draw3D();
        }

        {
            ImGui::Begin("System Settings");

            static bool s_sleep{};;
            ImGui::Checkbox("Sleep", &s_sleep);

            if (s_sleep)
            {
                System::Sleep(500);
            }

            ImGui::Checkbox("Upscaling Enabled", &s_upscalingEnabled);

            ImGui::End();
        }
    }

private:
};

void Demo_ShadertoyPS()
{
    Demo_ShadertoyPS_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
