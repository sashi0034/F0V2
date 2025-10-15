#include "pch.h"
#include "Demo_Shadertoy.h"

#include "imgui/imgui.h"
#include "TY/ComputeDispatcher.h"

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

            ComputeShader shadertoy_cs{ShaderParams::CS("asset/shader/shadertoy_cs.hlsl")};
        } shader;

        struct
        {
            ConstantBufferWrapper<Shadertoy_b10> shadertoy{};
        } cb;
    };

    InlineComponent<Resource_Shadertoy> s_rsc{};
}

struct Demo_Shadertoy_impl
{
    RenderTarget m_lowResolution{};

    TextureDrawer m_lowResolutionDrawer{};

    ComputeDispatcher m_csDispatcher{};

    Demo_Shadertoy_impl()
    {
        MainGamepad.registerMapping(GamepadMapping::FromTomlFile("asset/gamepad.toml"));

        m_lowResolution =
            RenderTargetParams()
            .setSize(Scene::Size() * 0.5)
            .setClearColor(ColorF32{0.0f, 1.0f})
            .setAllowUav(true);

        m_lowResolutionDrawer =
            TextureDrawerParams{}
            .setTexture(m_lowResolution.asTexture())
            .setShader(s_rsc->shader.default2d);

        m_csDispatcher =
            ComputeDispatcherParams{}
            .setCS(s_rsc->shader.shadertoy_cs)
            .setCbv({s_rsc->cb.shadertoy})
            .setUav({m_lowResolution.asUnorderedTexture()});
    }

    // -----------------------------------------------

    void draw3D()
    {
        const Float2 renderTargetSize = Float2{m_lowResolutionDrawer.size()};
        s_rsc->cb.shadertoy->g_screenResolution = renderTargetSize;
        s_rsc->cb.shadertoy->g_mousePosition = Mouse::PosF() * (renderTargetSize / Scene::Size().cast<Float2>());
        s_rsc->cb.shadertoy.upload();

        const Size threadGroup = (renderTargetSize.asPoint() + Size{7, 7}) / 8;
        m_csDispatcher.dispatchToDraw(threadGroup.x, threadGroup.y);
    }

    void Update()
    {
        draw3D();

        m_lowResolutionDrawer.as2D().resized(Scene::Size()).draw(Float2{});

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
};

void Demo_Shadertoy()
{
    Demo_Shadertoy_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
