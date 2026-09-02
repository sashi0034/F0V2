#include "pch.h"
#include "GimmickTextureDrawer.h"

#include "Asset.generated.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/DynamicBinding.h"
#include "TY/GameTime.h"
#include "TY/GenericModelBufferTemplates.h"
#include "TY/ModelDrawer.h"

using namespace Race;

namespace
{
    struct Gimmick_b10
    {
        float g_time;
    };
}

struct GimmickTextureDrawer::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"GimmickTextureDrawer";
#endif
    ActorContainer m_children{};

    Gimmick_b10 m_cb10{};

    GenericModelDrawer m_boostPadDrawer{};

    GenericModelDrawer m_jumpPadDrawer{};

    GenericModelDrawer m_pitZoneDrawer{};

    int m_frameCount{};

    void Init()
    {
        const auto model = std::make_shared<SingleShapeModelBuffer>(6);

        m_boostPadDrawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(model)
            .setVertexInput({})
            .setOptions(GraphicsOptions())
            .setShader(Asset_shader::gimmick_boost_pad)
            .setDynamicCbvCount(1)
        };

        m_jumpPadDrawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(model)
            .setVertexInput({})
            .setOptions(GraphicsOptions())
            .setShader(Asset_shader::gimmick_jump_pad)
            .setDynamicCbvCount(1)
        };

        m_pitZoneDrawer = GenericModelDrawer{
            GenericModelDrawerParams{}
            .setModel(model)
            .setVertexInput({})
            .setOptions(GraphicsOptions())
            .setShader(Asset_shader::gimmick_pit_zone)
            .setDynamicCbvCount(1)
        };

        drawGimmickTexture();
    }

private:
    void update() override
    {
        m_children.updateEach();

        m_cb10.g_time += InGameDeltaTime();

        m_frameCount++;
        if ((m_frameCount % 5) == 0)
        {
            drawGimmickTexture();
        }
    }

    // TODO: テクスチャのミップ対応
    // - MipSlice を変えて UAV を作成し、ComputeShader で書き込む
    void drawGimmickTexture()
    {
        const auto cbv = DynamicBinding::UploadDynamicCbv(m_cb10);

        // TODO: カメラから本当に見えるものだけ描画したい

        {
            const auto bind = g_sharedState->gimmickTextures.boostPad.scopedClearBind();
            DynamicBinding::SetDynamicCbv(10, cbv);
            m_boostPadDrawer.draw();
        }

        {
            const auto bind = g_sharedState->gimmickTextures.jumpPad.scopedClearBind();
            DynamicBinding::SetDynamicCbv(10, cbv);
            m_jumpPadDrawer.draw();
        }

        {
            const auto bind = g_sharedState->gimmickTextures.pitZone.scopedClearBind();
            DynamicBinding::SetDynamicCbv(10, cbv);
            m_pitZoneDrawer.draw();
        }
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Race
{
    GimmickTextureDrawer::GimmickTextureDrawer() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void GimmickTextureDrawer::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> GimmickTextureDrawer::asActor() const
    {
        return p_impl;
    }
}
