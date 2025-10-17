#include "pch.h"
#include "Demo_FSR1.h"

#include "TY/ComputeDispatcher.h"
#include "TY/ConstantBufferWrapper.h"
#include "TY/GenericModelBuffer.h"
#include "TY/GenericModelDrawer.h"
#include "TY/Graphics3D.h"
#include "TY/Mouse.h"
#include "TY/RenderTarget.h"
#include "TY/Shader.h"
#include "TY/System.h"
#include "TY/Scene.h"
#include "TY/TextureDrawer.h"

#define A_CPU
#include "asset/fsr1/ffx_a.h"
#include "asset/fsr1/ffx_fsr1.h"

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
}

struct Demo_FSR1_impl
{
    // シェーダー
    ComputeShader m_easuShader{ShaderParams::CS("asset/fsr1/fsr1_easu.hlsl")};

    ComputeDispatcher m_easuDispatcher{};

    ComputeShader m_rcasShader{ShaderParams::CS("asset/fsr1/fsr1_rcas.hlsl")};

    ComputeDispatcher m_rcasDispatcher{};

    GraphicsShader m_default2d{GraphicsShader::VS_PS("asset/shader/default2d.hlsl")};

    // リソース
    RenderTarget m_inputRT{
        RenderTargetParams{}
        .setRtvAndClearColor(RtvParams{}.setSize(Scene::Size() * 0.5).setClearColor(ColorF32{0.2f, 0.3f, 0.4f, 1.0f}))
    };

    UnorderedRenderTargetTexture m_upscaledTex{};

    UnorderedRenderTargetTexture m_sharpenedTex{};

    TextureDrawer m_upscalingDrawer{};

    struct EasuCB
    {
        std::array<uint32_t, 4> Const0{};
        std::array<uint32_t, 4> Const1{};
        std::array<uint32_t, 4> Const2{};
        std::array<uint32_t, 4> Const3{};
    };

    ConstantBufferWrapper<EasuCB> m_easuCB;

    struct RcasCB
    {
        std::array<uint32_t, 4> Const0{};
    };

    ConstantBufferWrapper<RcasCB> m_rcasCB;

    // -----------------------------------------------

    struct
    {
        GraphicsShader shadertoy_ps{GraphicsShader::VS_PS("asset/shader/shadertoy_ps.hlsl")};
    } m_shaders;

    GenericModelDrawer m_toyDrawer{};

    ConstantBufferWrapper<Shadertoy_b10> m_toyCB{};

    TextureDrawer m_defaultScalingDrawer{};

    Demo_FSR1_impl()
    {
        m_upscaledTex = RenderTargetTextureParams()
                        .setSize(Scene::Size())
                        .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

        m_sharpenedTex = RenderTargetTextureParams()
                         .setSize(Scene::Size())
                         .setFormat(DXGI_FORMAT_R16G16B16A16_FLOAT);

        // -----------------------------------------------

        m_toyDrawer = GenericModelDrawerParams{}
                      .setModel(std::make_unique<ToyModelBuffer>())
                      .setVertexInput({})
                      .setShader(m_shaders.shadertoy_ps)
                      .setOptions(GraphicsOptions{})
                      .setCbv10AndLater({m_toyCB});

        m_upscalingDrawer = TextureDrawer{
            TextureDrawerParams{}
            .setShader(m_default2d)
            .setTexture(m_sharpenedTex)
        };

        m_easuDispatcher = ComputeDispatcher{
            ComputeDispatcherParams{}
            .setCS(m_easuShader)
            .setCbv({m_easuCB})
            .setSrv({m_inputRT.asTexture()})
            .setUav({m_upscaledTex})
        };

        m_rcasDispatcher = ComputeDispatcher{
            ComputeDispatcherParams{}
            .setCS(m_rcasShader)
            .setCbv({m_rcasCB})
            .setSrv({m_upscaledTex})
            .setUav({m_sharpenedTex})
        };

        m_defaultScalingDrawer = TextureDrawer(TextureDrawerParams{}
                                               .setShader(m_default2d)
                                               .setTexture({m_inputRT.asTexture()}));
    }

    void DispatchEasu()
    {
        EasuCB cb{};
        FsrEasuCon(reinterpret_cast<AU1*>(&cb.Const0),
                   reinterpret_cast<AU1*>(&cb.Const1),
                   reinterpret_cast<AU1*>(&cb.Const2),
                   reinterpret_cast<AU1*>(&cb.Const3),
                   static_cast<AF1>(m_inputRT.size().x),
                   static_cast<AF1>(m_inputRT.size().y),
                   static_cast<AF1>(m_inputRT.size().x),
                   static_cast<AF1>(m_inputRT.size().y),
                   static_cast<AF1>(Scene::Size().x),
                   static_cast<AF1>(Scene::Size().y));
        m_easuCB.uploadValue(cb);

        m_upscaledTex.computeBarrierStart();

        int groupsX = (Scene::Size().x + 15) / 16;
        int groupsY = (Scene::Size().y + 15) / 16;
        m_easuDispatcher.dispatchToDraw(groupsX, groupsY, 1);

        m_upscaledTex.computeBarrierEnd();
    }

    inline static float s_rcasAttenuation{};

    void DispatchRcas()
    {
        RcasCB cb{};
        FsrRcasCon(reinterpret_cast<AU1*>(&cb.Const0), s_rcasAttenuation);
        m_rcasCB.uploadValue(cb);

        m_sharpenedTex.computeBarrierStart();

        int groupsX = (Scene::Size().x + 15) / 16;
        int groupsY = (Scene::Size().y + 15) / 16;
        m_rcasDispatcher.dispatchToDraw(groupsX, groupsY, 1);

        m_sharpenedTex.computeBarrierEnd();
    }

    void Update()
    {
        static bool s_fsrEnabled = true;

        // 低解像度で描画
        {
            auto bind = m_inputRT.scopedBind();
            m_toyCB->g_screenResolution = Float2{m_inputRT.size()};
            m_toyCB->g_mousePosition = Mouse::PosF() * 0.5f;
            m_toyCB->g_jitterOffset = Float2{}; // TODO
            m_toyCB.upload();

            m_toyDrawer.draw();
        }

        if (s_fsrEnabled)
        {
            DispatchEasu();
            DispatchRcas();
            m_upscalingDrawer.as2D().resized(Scene::Size()).draw(Float2{});
        }
        else
        {
            m_defaultScalingDrawer
                .as2D()
                .resized(Scene::Size())
                .draw(Float2{});
        }

        ImGui::Begin("FSR1 Settings");
        ImGui::Checkbox("Enable FSR1", &s_fsrEnabled);
        ImGui::SliderFloat("RCAS Attenuation", &s_rcasAttenuation, 0.0f, 2.0f);
        ImGui::End();
    }
};

void Demo_FSR1()
{
    Scene::RequestResize(Size{1920, 1080});
    // System::Update();

    Demo_FSR1_impl impl{};

    while (System::Update())
    {
        impl.Update();
    }
}
