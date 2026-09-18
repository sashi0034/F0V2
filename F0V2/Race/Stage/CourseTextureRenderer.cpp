#include "pch.h"
#include "CourseTextureRenderer.h"

#include "Asset.generated.h"
#include "Race/Common/CourseTextureKind.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/DynamicBinding.h"
#include "TY/DynamicTexture.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/Image.h"
#include "TY/ModelDrawer.h"
#include "TY/Palette.h"
#include "TY/RenderTarget.h"

using namespace Race;

namespace
{
    struct CourseTexture_b10
    {
        float g_time;
    };

    //  スタートラインの市松模様
    Image createStartingLineImage()
    {
        constexpr int half = 32;
        Image image{Size{half * 2, half * 2}};
        for (int y = 0; y < image.size().x; ++y)
        {
            for (int x = 0; x < image.size().y; ++x)
            {
                const bool isWhite = (x / half + y / half) % 2 == 0;
                image[{x, y}] = (isWhite ? Palette::White : Palette::Black).toColorU8();
            }
        }

        return image;
    }

    struct CourseRenderTextureConfig
    {
        int size; // 0 なら描画対象外
        int updateInterval; // 何フレームおきに描き直すか
    };

    CourseRenderTextureConfig getRenderTextureConfigOf(CourseTextureKind kind)
    {
        switch (kind)
        {
        case CourseTextureKind::None: return {};
        case CourseTextureKind::StartingLine: return {};
        case CourseTextureKind::RoadTop: return {.size = 512, .updateInterval = 2};
        case CourseTextureKind::RoadBottom: return {.size = 256, .updateInterval = 20};
        case CourseTextureKind::RoadSide: return {.size = 256, .updateInterval = 10};
        case CourseTextureKind::BoostPad: return {.size = 128, .updateInterval = 5};
        case CourseTextureKind::JumpPad: return {.size = 128, .updateInterval = 5};
        case CourseTextureKind::PitZone: return {.size = 256, .updateInterval = 5};
        default: break;
        }

        assert(false);
        return {};
    }

    const std::array<CourseRenderTextureConfig, CourseTextureCount>& getRenderTextureConfigs()
    {
        static const std::array<CourseRenderTextureConfig, CourseTextureCount> s_configs = []
        {
            std::array<CourseRenderTextureConfig, CourseTextureCount> result{};

            for (int i = 0; i < CourseTextureCount; ++i)
            {
                result[i] = getRenderTextureConfigOf(static_cast<CourseTextureKind>(i));
            }

            return result;
        }();

        return s_configs;
    }

    GraphicsShader getShaderOf(CourseTextureKind kind)
    {
        const auto courseShader = [](const std::string& psEntryPoint)
        {
            const auto& path = Asset_shader::prerender_course1.path();
            return GraphicsShader{
                .vs = VertexShader{path, "VS"},
                .ps = PixelShader{path, psEntryPoint},
            };
        };

        switch (kind)
        {
        case CourseTextureKind::RoadTop: return courseShader("PS_RoadTop");
        case CourseTextureKind::RoadBottom: return courseShader("PS_RoadBottom");
        case CourseTextureKind::RoadSide: return courseShader("PS_RoadSide");
        case CourseTextureKind::BoostPad: return Asset_shader::gimmick_boost_pad;
        case CourseTextureKind::JumpPad: return Asset_shader::gimmick_jump_pad;
        case CourseTextureKind::PitZone: return Asset_shader::gimmick_pit_zone;
        default: break;
        }

        assert(false);
        return GraphicsShader{};
    }
}

struct CourseTextureRenderer::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"CourseTextureRenderer";
#endif
    ActorContainer m_children{};

    CourseTexture_b10 m_cb10{};

    std::array<RenderTarget, CourseTextureCount> m_renderTargets{};

    std::array<GenericModelDrawer, CourseTextureCount> m_drawers{};

    int m_frameCount{};

    void Init()
    {
        const auto model = std::make_shared<SingleShapeModelBuffer>(6);

        DynamicTexture startingLineTexture = DynamicTexture{createStartingLineImage()};
        g_sharedState->courseTextures[static_cast<int>(CourseTextureKind::StartingLine)] = startingLineTexture;

        const auto& configs = getRenderTextureConfigs();

        for (int i = 0; i < CourseTextureCount; ++i)
        {
            const auto& config = configs[i];
            if (config.size == 0) continue;

            assert(config.updateInterval >= 1);

            m_renderTargets[i] =
                RenderTargetParams{}
                .setRtv(
                    RtvParams{}
                    .setSize(Size::One() * config.size)
                    .setClearColor(ColorF32{1.0f, 1.0f})
                    .enableFullMipLevels());

            g_sharedState->courseTextures[i] = m_renderTargets[i].getFrontRtv();

            m_drawers[i] = GenericModelDrawer{
                GenericModelDrawerParams{}
                .setModel(model)
                .setVertexInput({})
                .setOptions(GraphicsOptions())
                .setShader(getShaderOf(static_cast<CourseTextureKind>(i)))
                .setDynamicCbvCount(1)
            };
        }

        renderTextures(true);
    }

private:
    void update() override
    {
        m_children.updateEach();

        m_cb10.g_time += InGameDeltaTime();

        m_frameCount++;

        renderTextures();
    }

    void renderTextures(bool renderAll = false)
    {
        const auto cbv = DynamicBinding::UploadDynamicCbv(m_cb10);

        const auto& configs = getRenderTextureConfigs();

        // TODO: カメラから本当に見えるものだけ描画したい

        for (int i = 0; i < CourseTextureCount; ++i)
        {
            if (m_renderTargets[i].isEmpty()) continue;

            if (not renderAll && (m_frameCount % configs[i].updateInterval) != 0) continue;

            const auto bind = m_renderTargets[i].scopedClearBind();
            DynamicBinding::SetDynamicCbv(10, cbv);
            m_drawers[i].draw();
        }
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    CourseTextureRenderer::CourseTextureRenderer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CourseTextureRenderer::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> CourseTextureRenderer::asActor() const
    {
        return p_impl;
    }
}
