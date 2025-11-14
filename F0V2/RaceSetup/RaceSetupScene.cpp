#include "pch.h"
#include "RaceSetupScene.h"

#include "Asset0.h"
#include "CourseFileInfo.h"
#include "Race/Common/AiRank.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/CourseSegmentBuilder.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/Gamepad.h"
#include "TY/Immediate2D.h"
#include "TY/ImmediateDrawer.h"
#include "TY/InputUtils.h"
#include "TY/KeyboardInput.h"
#include "TY/Palette.h"
#include "TY/Screen.h"
#include "TY/Utils.h"
#include "Util/Utilities.h"

using namespace RaceSetup;

namespace
{
    void drawItemRow(
        const RectF& rowRegion,
        const std::u32string& icon,
        const std::u32string& leftMessage,
        const std::u32string& rightMessage,
        int itemIndex,
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
        Immediate2D_Text::MPlus1_Sdf(icon)
            .setSize(24.0f)
            .setPosition(columnRegions[0].middleLeft().movedX(20.0f), Alignment9::MiddleLeft)
            .setColor(ColorF32{isActive ? 0.9f : 0.5f})
            .pushAuto();

        Immediate2D_Text::MPlus1_Sdf(leftMessage)
            .setSize(24.0f)
            .setPosition(columnRegions[0].center(), Alignment9::MiddleCenter)
            .setColor(ColorF32{isActive ? 0.9f : 0.5f})
            .pushAuto();

        Immediate2D_Text::MPlus1_Sdf(ToUtf32(std::format("[{}]", itemIndex + 1)))
            .setSize(24.0f)
            .setPosition(columnRegions[1].middleRight().movedX(-64.0f), Alignment9::MiddleRight)
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

    struct
    {
        int aiRank{};
        int courseIndex{};
    } s_selectedItem;
}

struct RaceSetupScene::Impl : ActorBase
{
#if defined(_DEBUG)
    std::u32string m_debugName = U"RaceSetupScene";
#endif

    ActorContainer m_children{};

    int m_rowIndex{};

    bool m_confirmed{};

    void Init()
    {
    }

private:
    void update() override
    {
        handleInput();

        Immediate2D::Rect{Screen::RectF()}
            .setColor(ColorF32{0.1f})
            .pushAuto();

        const auto hudRegion = Screen::RectF().stretched(-400.0f, -320.0f);

        const auto [titleBarRegion, contentRegion] =
            hudRegion.separate(80.0f, Direction4::Up);

        Immediate2D::RoundRect{hudRegion}
            .setColor(ColorF32{0.9f})
            .setRoundness(20.0f)
            .pushAuto();

        Immediate2D::RoundRect{titleBarRegion}
            .setColor(ColorF32{0.2f})
            .pushAuto();

        Immediate2D_Text::MPlus1_Sdf(U"\U000F0B6E レース選択")
            .setSize(32.0f)
            .setPosition(titleBarRegion.center(), Alignment9::MiddleCenter)
            .setColor(ColorF32{0.9f})
            .pushAuto();

        const auto lineRegions =
            Util::SliceRectByLength(contentRegion.stretched(-20.0f, -32.0f), 48.0f, Direction2::Vertical);

        constexpr std::array aiDisplayItems = {
            U"やさしい",
            U"ふつう",
            U"強い",
            U"最強",
        };
        static_assert(aiDisplayItems.size() == static_cast<int>(Race::AiRank::Max));

        drawItemRow(lineRegions[0],
                    U"\U000F169E",
                    U"AI つよさ",
                    aiDisplayItems[s_selectedItem.aiRank],
                    s_selectedItem.aiRank,
                    m_rowIndex == 0);

        const auto& courseInfo = GetAllCourseFileInfos()[s_selectedItem.courseIndex];
        drawItemRow(lineRegions[1],
                    U"\U000F0982",
                    U"コース",
                    courseInfo.displayName,
                    s_selectedItem.courseIndex,
                    m_rowIndex == 1);

        std::u32string difficultyText{U"難易度:"};
        for (int i = 0; i < courseInfo.difficulty; ++i)
        {
            difficultyText += U" \U000F04CE";
        }

        drawDescriptionRow(lineRegions[2], difficultyText);

        drawDescriptionRow(lineRegions[3], courseInfo.description1);

        drawDescriptionRow(lineRegions[4], courseInfo.description2);

        // 開始ボタン
        {
            bool isActive = m_rowIndex == 2;
            const auto& lastRowRegion = lineRegions[lineRegions.size() - 1];
            Immediate2D::RoundRect{RectF{lastRowRegion.center(), Alignment9::MiddleCenter, SizeF{400.0f, 48.0f}}}
                .setColor(isActive ? Palette::RoyalBlue : ColorF32{1.0f}) // TODO
                .pushAuto();
            Immediate2D_Text::MPlus1_Sdf(U"\U000F1807 レース開始")
                .setSize(24.0f)
                .setPosition(lastRowRegion.center(), Alignment9::MiddleCenter)
                .setColor(isActive ? ColorF32{1.0f} : ColorF32{0.1f}) // TODO
                .pushAuto();
        }

        ImmediateDrawer::Global().draw();
    }

    void handleInput()
    {
        if (m_confirmed)
        {
            return;
        }

        std::optional<Direction4> dirOpt{};
        std::optional<bool> confirmTriggered{};

        if (IsUsingGamepad())
        {
            dirOpt = GamepadUtils::GetTriggeredDpad();
            confirmTriggered = MainGamepad.a().down;
        }
        else
        {
            dirOpt = KeyboardUtils::GetTriggeredArrowOrWASD();
            confirmTriggered = KeySpace.down();
        }

        Point dir{};
        if (dirOpt.has_value())
        {
            dir = DirectionToPoint(*dirOpt);
        }

        if (m_rowIndex == 0)
        {
            s_selectedItem.aiRank =
                Modulo(s_selectedItem.aiRank + dir.x, static_cast<int>(Race::AiRank::Max));
        }
        else if (m_rowIndex == 1)
        {
            s_selectedItem.courseIndex =
                Modulo<int>(s_selectedItem.courseIndex + dir.x, GetAllCourseFileInfos().size());
        }
        else if (m_rowIndex == 2)
        {
            m_confirmed = confirmTriggered.value_or(false);
            if (m_confirmed)
            {
                // NOTE: コースの読み込みは呼び出し側で行う
                Race::g_sharedState->aiRank = static_cast<Race::AiRank>(s_selectedItem.aiRank);
                return;
            }
        }

        m_rowIndex += dir.y;
        m_rowIndex = Math::Clamp(m_rowIndex, 0, 2);
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

    bool RaceSetupScene::isConfirmed() const
    {
        return p_impl->m_confirmed;
    }

    std::string RaceSetupScene::selectedCourseFilepath() const
    {
        const auto& courseInfo = GetAllCourseFileInfos()[s_selectedItem.courseIndex];
        return courseInfo.filepath;
    }

    std::shared_ptr<ActorBase> RaceSetupScene::asActor() const
    {
        return p_impl;
    }
}
