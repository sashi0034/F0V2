#include "pch.h"
#include "DebugEditor.h"

#include "Asset0.h"
#include "ColorPalette.h"
#include "DebugUI.h"
#include "TY/ActorContainer.h"
#include "TY/KeyboardInput.h"
#include "TY/Scene.h"
#include "TY/ShapeDrawer.h"
#include "TY/Utils.h"
#include "TY_Extension/GameObjectHierarchy.h"
#include "TY_Extension/SerializeTransform.h"
#include "Util/Utilities.h"

using namespace Combat;

namespace
{
    void gameObjectEditor(const GameObjectHierarchy::list_type& gameObjectList)
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

        // Shape2D::RoundRect{sliderRegion.stretched(0, -1)}
        //     .setColor(ColorF32{"#4F4F4F"})
        //     .pushAuto();

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

        // if (DebugUI::Button(operationRects[0].stretched(-1), U"Add"))
        // {
        //     gameObjectList.push_back(SerializeTransform{
        //         "Empty",
        //         {0, 0, 0},
        //         {0, 0, 0},
        //         {1, 1, 1}
        //     });
        // }

        // Shape2D_Text::MPlus1_24_Bitmap(U"デバッグ機能 ABC abc 012")
        //     .setPosition(Float2{10, 100})
        //     .setSize(100)
        //     .pushAuto();

        ShapeDrawer::Global().draw();
    }

    // -----------------------------------------------
}

struct DebugEditor::Impl : ActorBase
{
    ActorContainer m_children{};

    void init()
    {
    }

    void update() override
    {
        m_children.updateEach();

        gameObjectEditor(GlobalGameObjectHierarchy().list());
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();
    }
};

namespace Combat
{
    DebugEditor::DebugEditor()
        : p_impl(std::make_shared<Impl>())
    {
    }

    void DebugEditor::init()
    {
        p_impl->init();
    }

    std::shared_ptr<ActorBase> DebugEditor::asActor() const
    {
        return p_impl;
    }
}
