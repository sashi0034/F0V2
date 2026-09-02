#include "pch.h"

#define A_CPU
#include "asset/shader/fsr1/ffx_a.h"
#include "asset/shader/fsr1/ffx_fsr1.h"

#include "RaceDrawUpscaler.h"

#include "Asset.generated.h"
#include "RaceSharedState.h"
#include "TY/ComputeDispatcher.h"
#include "TY/DynamicBinding.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/GenericModelDrawer.h"
#include "TY/RenderTarget.h"
#include "TY/RenderTargetTexture.h"
#include "TY/Screen.h"

using namespace Race;

namespace
{
    struct CheapAA_b10
    {
        Float2 g_outputResolution;
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

    class Fsr1Upscaler
    {
    public:
        void Init(const TextureHandle& input, const Size& outputSize)
        {
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
                .setCS(Asset_shader::fsr1_easu_cs)
                .setDynamicCbvCount(1)
                .setSrv({input})
                .setUav({m_easuTexture})
            };

            m_rcasDispatcher = ComputeDispatcher{
                ComputeDispatcherParams{}
                .setCS(Asset_shader::fsr1_rcas_cs)
                .setDynamicCbvCount(1)
                .setSrv({m_easuTexture})
                .setUav({m_rcasTexture})
            };
        }

        void Dispatch(const SizeF& inputSize, float sharpnessAttenuation)
        {
            // EASU
            {
                EasuCB cb{};
                FsrEasuCon(reinterpret_cast<AU1*>(&cb.Const0),
                           reinterpret_cast<AU1*>(&cb.Const1),
                           reinterpret_cast<AU1*>(&cb.Const2),
                           reinterpret_cast<AU1*>(&cb.Const3),
                           static_cast<AF1>(inputSize.x),
                           static_cast<AF1>(inputSize.y),
                           static_cast<AF1>(m_outputSize.x),
                           static_cast<AF1>(m_outputSize.y),
                           static_cast<AF1>(m_outputSize.x),
                           static_cast<AF1>(m_outputSize.y));
                int groupsX = (m_outputSize.x + 15) / 16;
                int groupsY = (m_outputSize.y + 15) / 16;
                DynamicBinding::SetDynamicCbv(0, cb);
                m_easuDispatcher.dispatch(groupsX, groupsY, 1);
            }

            // RCAS
            {
                RcasCB cb{};
                FsrRcasCon(reinterpret_cast<AU1*>(&cb.Const0), sharpnessAttenuation);
                int groupsX = (m_outputSize.x + 15) / 16;
                int groupsY = (m_outputSize.y + 15) / 16;
                DynamicBinding::SetDynamicCbv(0, cb);
                m_rcasDispatcher.dispatch(groupsX, groupsY, 1);
            }
        }

        TextureHandle OutputTexture() const
        {
            return m_rcasTexture;
        }

    private:
        // Size m_inputSize{};
        Size m_outputSize{};

        UnorderedRenderTargetTexture m_easuTexture{};
        ComputeDispatcher m_easuDispatcher{};

        UnorderedRenderTargetTexture m_rcasTexture{};
        ComputeDispatcher m_rcasDispatcher{};
    };
}

struct RaceDrawUpscaler::Impl
{
    GenericModelDrawer m_aaDrawer{};
    RenderTarget m_aaTarget{};

    Fsr1Upscaler m_fsr1Upscaler{};

    void Init(const RenderTargetTexture& enderTexture)
    {
        m_aaDrawer =
            GenericModelDrawerParams{}
            .setModel(std::make_unique<SingleShapeModelBuffer>(6))
            .setVertexInput({})
            .setShader(Asset_shader::cheap_aa)
            .setOptions(GraphicsOptions{})
            .setDynamicCbvCount(1)
            .setSrv10AndLater({enderTexture});

        m_aaTarget =
            RenderTargetParams{}
            .setRtv(RtvParams{}
                    .setFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
                    .setSize(g_sharedState->gbufferTarget.size())
                    .setClearColor(ColorF32{0.0f, 0.0f}));

        m_fsr1Upscaler.Init(m_aaTarget.getFrontRtv(), enderTexture.size());
    }

    Immediate2D::Texture Upscale(float renderScale, bool fsrEnabled)
    {
        // AA
        {
            const CheapAA_b10 cb{
                .g_outputResolution = g_sharedState->gbufferTarget.size() * renderScale,
            };

            const auto bind = m_aaTarget.scopedClearBind();
            DynamicBinding::SetDynamicCbv(10, cb);
            m_aaDrawer.draw();
        }

        if (not fsrEnabled)
        {
            return Immediate2D::Texture(m_aaTarget.getFrontRtv())
                   .trimmed(Screen::SizeF() * renderScale)
                   .resized(Screen::Size());
        }

        // FSR
        {
            constexpr float k_sharpnessAttenuation = 0.0f;
            m_fsr1Upscaler.Dispatch(Screen::SizeF() * renderScale, k_sharpnessAttenuation);
        }

        return Immediate2D::Texture(m_fsr1Upscaler.OutputTexture());
    }
};

namespace Race
{
    RaceDrawUpscaler::RaceDrawUpscaler() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceDrawUpscaler::init(const RenderTargetTexture& renderTexture)
    {
        p_impl->Init(renderTexture);
    }

    Immediate2D::Texture RaceDrawUpscaler::upscale(float renderScale, bool fsrEnabled)
    {
        return p_impl->Upscale(renderScale, fsrEnabled);
    }
}
