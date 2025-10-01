#include "pch.h"
#include "EditorScene.h"

#include "Asset0.h"
#include "ColorPalette.h"
#include "EditorState.h"
#include "EditorPlayground.h"
#include "DebugUI.h"
#include "EditorNodeTool.h"
#include "GM/DebugService.h"
#include "Race/Common/CourseData.h"
#include "TY/ActorContainer.h"
#include "TY/Scene.h"
#include "TY/ShapeDrawer.h"
#include "TY_Extension/GameObjectHierarchy.h"
#include "Util/Utilities.h"

using namespace Editor;
using namespace Race;

namespace
{
    void hierarchWindow(const GameObjectHierarchy::list_type& gameObjectList)
    {
        const auto backgroundRegion = RectF{Scene::Rect().stretched(-10).bl(), Alignment9::BottomLeft, Size{400, 800}};

        Shape2D::RoundRect{backgroundRegion}
            .setColor(ColorPalette::EditorBackground)
            .setOutline({1.0f, ColorPalette::GrayOrange})
            .pushAuto();

        constexpr float lineLength = 28.0f;
        auto [headerRegion, contentRegion] = backgroundRegion.separate(lineLength, Direction4::Up);

        Shape2D::RoundRect{headerRegion.stretched(-1)}
            .setColor(ColorF32{ColorPalette::DarkOrange} * 1.05f)
            .pushAuto();

        Shape2D_Text::MPlus1_16_Bitmap(U"GameObject List")
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

        ShapeDrawer::Global().draw();
    }

    // -----------------------------------------------

    const std::string courseFilepath = "asset/edit/sandbox_course.toml";

    CourseData loadCourse()
    {
        CourseData result;

        try
        {
            auto tbl = toml::parse_file(courseFilepath);

            if (auto* arr = tbl["nodes"].as_array())
            {
                for (auto&& nodeVal : *arr)
                {
                    if (auto* nodeTbl = nodeVal.as_table())
                    {
                        if (auto* posArr = (*nodeTbl)["pos"].as_array())
                        {
                            if (posArr->size() == 3)
                            {
                                CourseNode node{};
                                node.pos.x = static_cast<float>((*posArr)[0].value_or(0.0));
                                node.pos.y = static_cast<float>((*posArr)[1].value_or(0.0));
                                node.pos.z = static_cast<float>((*posArr)[2].value_or(0.0));
                                result.nodes.push_back(node);
                            }
                        }
                    }
                }
            }
        }
        catch (const toml::parse_error& err)
        {
            std::cerr << "TOML parse error: " << err.description() << " at " << err.source().begin << "\n";
        }

        return result;
    }

    void saveCourse(const CourseData& course)
    {
        toml::table root{};
        toml::array nodesArr{};

        for (const auto& node : course.nodes)
        {
            toml::table nodeTbl;
            toml::array posArr{};
            posArr.push_back(node.pos.x);
            posArr.push_back(node.pos.y);
            posArr.push_back(node.pos.z);

            nodeTbl.insert("pos", std::move(posArr));
            nodesArr.push_back(std::move(nodeTbl));
        }

        root.insert("nodes", std::move(nodesArr));

        std::ofstream file(courseFilepath);
        file << root;
    }
}

struct EditorScene::Impl : ActorBase
{
    ActorContainer m_children{};

    EditorNodeTool m_debugNodeEditor{};

    EditorPlayground m_debugPlayground{};

    void init()
    {
        g_editorState->course = loadCourse();

        m_debugNodeEditor = m_children.birth(EditorNodeTool());
        m_debugNodeEditor.init();

        m_debugPlayground = m_children.birth(EditorPlayground());
        m_debugPlayground.init();
    }

    void update() override
    {
        m_children.updateEach();

        // hierarchWindow(GlobalGameObjectHierarchy().list());

        // -----------------------------------------------

        {
            ImGui::Begin("Editor");

            if (ImGui::Button("Reset Camera"))
            {
                m_debugPlayground.resetCamera();
            }

            ImGui::End();
        }
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();

        saveCourse(g_editorState->course);
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
