#include "pch.h"
#include "CourseRenderTextureDrawer.h"

#include "Asset.generated.h"
#include "Race/Common/CourseRenderTextureKind.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/DynamicBinding.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/ModelDrawer.h"

using namespace Race;

namespace
{
    struct CourseRenderTexture_b10
    {
        float g_time;
    };

    GraphicsShader GetShaderOf(CourseRenderTextureKind kind)
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
        case CourseRenderTextureKind::RoadTop: return courseShader("PS_RoadTop");
        case CourseRenderTextureKind::RoadBottom: return courseShader("PS_RoadBottom");
        case CourseRenderTextureKind::RoadSide: return courseShader("PS_RoadSide");
        case CourseRenderTextureKind::BoostPad: return Asset_shader::gimmick_boost_pad;
        case CourseRenderTextureKind::JumpPad: return Asset_shader::gimmick_jump_pad;
        case CourseRenderTextureKind::PitZone: return Asset_shader::gimmick_pit_zone;
        default: break;
        }

        assert(false);
        return GraphicsShader{};
    }
}

struct CourseRenderTextureDrawer::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"CourseRenderTextureDrawer";
#endif
    ActorContainer m_children{};

    CourseRenderTexture_b10 m_cb10{};

    std::array<GenericModelDrawer, CourseRenderTextureKindCount> m_drawers{};

    int m_frameCount{};

    void Init()
    {
        const auto model = std::make_shared<SingleShapeModelBuffer>(6);

        for (int i = 0; i < CourseRenderTextureKindCount; ++i)
        {
            m_drawers[i] = GenericModelDrawer{
                GenericModelDrawerParams{}
                .setModel(model)
                .setVertexInput({})
                .setOptions(GraphicsOptions())
                .setShader(GetShaderOf(static_cast<CourseRenderTextureKind>(i)))
                .setDynamicCbvCount(1)
            };
        }

        drawTextures();
    }

private:
    void update() override
    {
        m_children.updateEach();

        m_cb10.g_time += InGameDeltaTime();

        m_frameCount++;
        if ((m_frameCount % 5) == 0)
        {
            drawTextures();
        }
    }

    void drawTextures()
    {
        const auto cbv = DynamicBinding::UploadDynamicCbv(m_cb10);

        // TODO: カメラから本当に見えるものだけ描画したい

        for (int i = 0; i < CourseRenderTextureKindCount; ++i)
        {
            const auto bind = g_sharedState->courseRenderTextures[i].scopedClearBind();
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
    CourseRenderTextureDrawer::CourseRenderTextureDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void CourseRenderTextureDrawer::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> CourseRenderTextureDrawer::asActor() const
    {
        return p_impl;
    }
}
