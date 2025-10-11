#include "pch.h"
#include "EditorScene.h"

#include "Asset0.h"
#include "ColorPalette.h"
#include "EditorState.h"
#include "EditorPlayground.h"
#include "Util/DebugUI.h"
#include "EditorNodeTool.h"
#include "GM/DebugService.h"
#include "Race/Common/CourseData.h"
#include "TY/ActorContainer.h"
#include "TY/Scene.h"
#include "TY/ImmediateDrawer.h"
#include "TY_Extension/GameObjectHierarchy.h"
#include "Util/Utilities.h"

using namespace Editor;
using namespace Race;

namespace
{
#if 0
    void hierarchWindow(const GameObjectHierarchy::list_type& gameObjectList)
    {
        const auto backgroundRegion = RectF{Scene::Rect().stretched(-10).bl(), Alignment9::BottomLeft, Size{400, 800}};

        Immediate2D::RoundRect{backgroundRegion}
            .setColor(ColorPalette::EditorBackground)
            .setOutline({1.0f, ColorPalette::GrayOrange})
            .pushAuto();

        constexpr float lineLength = 28.0f;
        auto [headerRegion, contentRegion] = backgroundRegion.separate(lineLength, Direction4::Up);

        Immediate2D::RoundRect{headerRegion.stretched(-1)}
            .setColor(ColorF32{ColorPalette::DarkOrange} * 1.05f)
            .pushAuto();

        Immediate2D_Text::MPlus1_16_Bitmap(U"GameObject List")
            .setPosition(headerRegion.stretched(-5).middleLeft(), Alignment9::MiddleLeft)
            .pushAuto();

        const auto [operationRegion, contentRegion2] =
            contentRegion.stretched(-5).stretched(5, Direction4::Up).separate(lineLength, Direction4::Down);

        const auto [sliderRegion, listRegion] = contentRegion2.separate(10, Direction4::Left);

        const auto listRects = Util::SliceRectByLength(listRegion.stretched(-5), lineLength, Direction2::Vertical);

        static int s_listStart = 0;
        DebugUI::ListSlider(
            s_listStart, listRects.size(), gameObjectList.size(), sliderRegion.stretched(0, -1), contentRegion2);

        static int s_activeItem = -1;
        for (int i = 0; i < listRects.size(); ++i)
        {
            const int index = i + s_listStart;
            if (index >= gameObjectList.size()) break;

            const auto& r = listRects[i];
            if (DebugUI::ItemButton(r.stretched(-1), gameObjectList[index]->name(), s_activeItem == index))
            {
                s_activeItem = index;
            }
        }

        if (InRange(s_activeItem, 0, static_cast<int>(gameObjectList.size() - 1)))
        {
            gameObjectList[s_activeItem]->debugInspector();
        }

        const auto operationRects =
            Util::SliceRectByLength(operationRegion, operationRegion.w / 4, Direction2::Horizontal);

        ImmediateDrawer::Global().draw();
    }
#endif

    // -----------------------------------------------

    const std::string courseFilepath = "asset/edit/sandbox_course.toml";
}

struct EditorScene::Impl : ActorBase
{
    ActorContainer m_children{};

    EditorNodeTool m_debugNodeEditor{};

    EditorPlayground m_debugPlayground{};

    void init()
    {
        g_editorState->course = LoadCourseData(courseFilepath);

        m_debugNodeEditor = m_children.birth(EditorNodeTool());
        m_debugNodeEditor.init();

        m_debugPlayground = m_children.birth(EditorPlayground());
        m_debugPlayground.init();
    }

    void update() override
    {
        m_children.updateEach();

        // hierarchWindow(GlobalGameObjectHierarchy().list());
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();

        SaveCourseData(g_editorState->course, courseFilepath);
    }
};

namespace Editor
{
    EditorScene::EditorScene()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void EditorScene::init()
    {
        p_impl->init();
    }

    std::shared_ptr<ActorBase> EditorScene::asActor() const
    {
        return p_impl;
    }
}
