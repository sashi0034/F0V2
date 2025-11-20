#include "pch.h"
#include "EditorScene.h"

#include "Asset0.h"
#include "GamePalette.h"
#include "EditorState.h"
#include "EditorPlayground.h"
#include "Util/DebugUI.h"
#include "EditorNodeTool.h"
#include "GM/DebugService.h"
#include "Race/Common/CourseData.h"
#include "Race/Common/RaceDeferredShadingDrawer.h"
#include "Race/Common/RaceSharedState.h"
#include "TY/ActorContainer.h"
#include "TY/Screen.h"
#include "TY/ImmediateDrawer.h"
#include "TY_Extension/GameObjectHierarchy.h"
#include "Util/DebugTomlValue.h"
#include "Util/Utilities.h"

using namespace Editor;
using namespace Race;

namespace
{
#if 0
    void hierarchWindow(const GameObjectHierarchy::list_type& gameObjectList)
    {
        const auto backgroundRegion = RectF{Screen::Rect().stretched(-10).bl(), Alignment9::BottomLeft, Size{400, 800}};

        Immediate2D::RoundRect{backgroundRegion}
            .setColor(GamePalette::EditorBackground)
            .setOutline({1.0f, GamePalette::GrayOrange})
            .pushAuto();

        constexpr float lineLength = 28.0f;
        auto [headerRegion, contentRegion] = backgroundRegion.separate(lineLength, Direction4::Up);

        Immediate2D::RoundRect{headerRegion.stretched(-1)}
            .setColor(ColorF32{GamePalette::DarkOrange} * 1.05f)
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

    const std::string defaultCourseFilepath = "asset/course/sandbox_course.toml";
}

struct EditorScene::Impl : ActorBase
{
    ActorContainer m_children{};

    EditorNodeTool m_debugNodeEditor{};

    EditorPlayground m_debugPlayground{};

    std::string m_courseFilepath = defaultCourseFilepath;

    RaceDeferredShadingDrawer m_deferredShadingDrawer{};

    void init()
    {
        reloadCourseData();

        m_debugNodeEditor = m_children.birth(EditorNodeTool());
        m_debugNodeEditor.init();

        m_debugPlayground = m_children.birth(EditorPlayground());
        m_debugPlayground.init();

        m_deferredShadingDrawer.init();
    }

    void update() override
    {
        m_debugPlayground.applyCamera();

        m_children.updateEach();

        // hierarchWindow(GlobalGameObjectHierarchy().list());

        constexpr float renderScale = 1.0f;

        // GBuffer パス
        {
            g_sharedState->gbufferTarget.setViewport(RectF{Screen::SizeF() * renderScale});
            const auto bind = g_sharedState->gbufferTarget.scopedClearBind();

            m_debugNodeEditor.drawGBuffer();

            m_debugPlayground.drawGBuffer();
        }

        // レイマーチング & ディファードシェーディングパス
        {
            m_deferredShadingDrawer.draw(renderScale);

            Immediate2D::Texture{m_deferredShadingDrawer.getOutputTexture()}
                .resized(Screen::Size())
                .pushAuto();
        }

        // UI
        {
            m_debugNodeEditor.debugUI();
        }

        drawUI();
    }

    void killed() override
    {
        m_children.killEach();

        SaveCourseData(g_editorState->course, m_courseFilepath);
    }

private:
    void reloadCourseData()
    {
#if defined(_DEBUG)
        m_courseFilepath = GetDebugTomlValue<std::string>("fixed_course_path");
#endif

        g_editorState->course = LoadCourseData(m_courseFilepath);
    }

    void drawUI()
    {
        ImGui::Begin("Editor Scene");

        ImGui::Text(std::format("m_courseFilepath: {}", m_courseFilepath).c_str());

        if (ImGui::Button("Reload Course Data"))
        {
            reloadCourseData();
        }

        ImGui::End();
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
