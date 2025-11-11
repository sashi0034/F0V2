#include "pch.h"
#include "RaceSetupScene.h"

#include "Asset0.h"
#include "TY/ActorContainer.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/KeyboardInput.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "Util/Utilities.h"

using namespace RaceSetup;

namespace
{
    void drawItemRow(
        const RectF& rowRegion,
        const std::u32string& leftMessage,
        const std::u32string& rightMessage,
        bool isActive)
    {
        const auto region = rowRegion.stretched(-4.0f);
        Immediate2D::RoundRect{region}
            .setColor(isActive ? Palette::RoyalBlue : ColorF32{1.0f})
            .pushAuto();

        if (not isActive)
        {
            Immediate2D::Rect{region.separate(8.0f, Direction4::Left).first}
                .setColor(Palette::RoyalBlue)
                .pushAuto();
        }

        const auto columnRegions = Util::SliceRectByCount(region, 2, Direction2::Horizontal);
        Immediate2D_Text::MPlus1_Sdf(leftMessage)
            .setSize(24.0f)
            .setPosition(columnRegions[0].center(), Alignment9::MiddleCenter)
            .setColor(ColorF32{isActive ? 0.9f : 0.5f})
            .pushAuto();

        Immediate2D_Text::MPlus1_Sdf(rightMessage)
            .setSize(24.0f)
            .setPosition(columnRegions[1].center(), Alignment9::MiddleCenter)
            .setColor(ColorF32{isActive ? 1.0f : 0.1f})
            .pushAuto();

        // 三角形
        {
            const auto l = columnRegions[1].middleLeft().movedBy(20.0f, 0.0f);
            Immediate2D::Path{}
                .append(l).append(l.movedBy(12.0f, 8.0f)).append(l.movedBy(12.0f, -8.0f))
                .setThickness(4.0f)
                .setColor(ColorF32{isActive ? 0.9f : 0.5f})
                .asCycle()
                .pushAuto();

            const auto r = columnRegions[1].middleRight().movedBy(-20.0f, 0.0f);
            Immediate2D::Path{}
                .append(r).append(r.movedBy(-12.0f, 8.0f)).append(r.movedBy(-12.0f, -8.0f))
                .setThickness(4.0f)
                .setColor(ColorF32{isActive ? 0.9f : 0.5f})
                .asCycle()
                .pushAuto();
        }
    }

    void drawDescriptionRow(const RectF& region, const std::u32string& desc)
    {
        Immediate2D_Text::MPlus1_Sdf(desc)
            .setSize(24.0f)
            .setPosition(region.middleLeft().movedX(80.0f), Alignment9::MiddleLeft)
            .setColor(ColorF32{0.5f})
            .pushAuto();
    }
}

struct RaceSetupScene::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"RaceSetupScene";
#endif

    ActorContainer m_children{};

    void Init()
    {
    }

private:
    void update() override
    {
        Immediate2D::Rect{Screen::RectF()}
            .setColor(ColorF32{0.1f})
            .pushAuto();

        const auto hudRegion = Screen::RectF().stretched(-400.0f, -280.0f);

        const auto [titleBarRegion, contentRegion] =
            hudRegion.separate(80.0f, Direction4::Up);

        Immediate2D::RoundRect{hudRegion}
            .setColor(ColorF32{0.9f})
            .setRoundness(20.0f)
            .pushAuto();

        Immediate2D::RoundRect{titleBarRegion}
            .setColor(ColorF32{0.2f})
            .pushAuto();

        Immediate2D_Text::MPlus1_Sdf(U"レース選択")
            .setSize(32.0f)
            .setPosition(titleBarRegion.center(), Alignment9::MiddleCenter)
            .setColor(ColorF32{0.9f})
            .pushAuto();

        const auto lineRegions =
            Util::SliceRectByLength(contentRegion.stretched(-20.0f, -32.0f), 48.0f, Direction2::Vertical);

        drawItemRow(lineRegions[0], U"AI つよさ", U"ふつう", KeySpace.pressed());

        drawItemRow(lineRegions[1], U"コース", U"惑星ルビコン 6", KeySpace.pressed());

        drawDescriptionRow(lineRegions[2], U"ここは強化人間-san がいる星です。");

        drawDescriptionRow(lineRegions[3], U"難易度: ●★☆");

        Immediate2D::RoundRect{RectF{lineRegions[5].center(), Alignment9::MiddleCenter, SizeF{400.0f, 48.0f}}}
            .setColor(false ? Palette::RoyalBlue : ColorF32{1.0f}) // TODO
            .pushAuto();
        Immediate2D_Text::MPlus1_Sdf(U"レース開始 !")
            .setSize(24.0f)
            .setPosition(lineRegions[5].center(), Alignment9::MiddleCenter)
            .setColor(false ? ColorF32{1.0f} : ColorF32{0.1f}) // TODO
            .pushAuto();

        ImmediateDrawer::Global().draw();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace RaceSetup
{
    RaceSetupScene::RaceSetupScene() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void RaceSetupScene::init()
    {
        p_impl->Init();
    }

    std::shared_ptr<ActorBase> RaceSetupScene::asActor() const
    {
        return p_impl;
    }
}
