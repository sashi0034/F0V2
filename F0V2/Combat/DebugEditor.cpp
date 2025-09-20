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
#include "TY_Extension/SerializeTransform.h"
#include "Util/Utilities.h"

using namespace Combat;

namespace
{
    inline bool imguiFloat3(const char* label, Float3& v, float resetValue = 0.0f, float columnWidth = 100.0f)
    {
        bool changed = false;

        if (ImGui::BeginTable(label, 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, columnWidth);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

            ImGui::TableNextRow();

            // Label
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label);

            // Controls
            ImGui::TableSetColumnIndex(1);

            float fullWidth = ImGui::GetContentRegionAvail().x;
            float itemWidth = fullWidth / 3.0f;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4, 0});

            auto drawAxis = [&](const char* label, float& value, const ImVec4& color)
            {
                ImGui::PushItemWidth(itemWidth - 20.0f);

                // 軸ラベル (色付き)
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(label);
                ImGui::SameLine();
                ImGui::PopStyleColor();

                // 値入力
                changed |= ImGui::DragFloat(std::string("##").append(label).c_str(), &value, 0.1f, 0.0f, 0.0f, "%.3f");

                ImGui::PopItemWidth();
                ImGui::SameLine();
            };
            drawAxis("X", v.x, {1.0f, 0.4f, 0.4f, 1.0f}); // 赤
            drawAxis("Y", v.y, {0.4f, 1.0f, 0.4f, 1.0f}); // 緑
            drawAxis("Z", v.z, {0.4f, 0.7f, 1.0f, 1.0f}); // 青

            ImGui::PopStyleVar();

            ImGui::EndTable();
        }

        return changed;
    }

    void imguiTransformInspector(SerializeTransform& tr)
    {
        ImGui::Begin("Transform Inspector");

        // Tag
        char buffer[256];
        strcpy_s(buffer, tr.tag.c_str());
        ImGui::Text("Tag:");
        ImGui::SameLine();
        if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        {
            tr.tag = buffer;
        }

        ImGui::Separator();

        // 各プロパティ
        if (imguiFloat3("Position", tr.position, 0.0f))
        {
        }

        if (imguiFloat3("Rotation", tr.rotation, 0.0f))
        {
        }

        if (imguiFloat3("Scale", tr.scale, 1.0f))
        {
        }

        ImGui::End();
    }
}

namespace
{
    void transformListEditor(Array<SerializeTransform>& transformList)
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

        Shape2D_Text::MPlus1_16_Bitmap(U"Transform Editor")
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
            s_listStart, listRects.size(), transformList.size(), sliderRegion.stretched(0, -1), contentRegion2);

        static int s_activeItem = -1;
        for (int i = 0; i < listRects.size(); ++i)
        {
            const int index = i + s_listStart;
            if (index >= transformList.size()) break;

            const auto& r = listRects[i];
            if (DebugUI::ItemButton(r.stretched(-1), ToUtf32(transformList[index].tag), s_activeItem == index))
            {
                s_activeItem = index;
            }
        }

        if (InRange(s_activeItem, 0, static_cast<int>(transformList.size() - 1)))
        {
            imguiTransformInspector(transformList[s_activeItem]);

            if (KeyDelete.down())
            {
                transformList.erase(transformList.begin() + s_activeItem);
            }
        }

        const auto operationRects =
            Util::SliceRectByLength(operationRegion, operationRegion.w / 4, Direction2::Horizontal);

        if (DebugUI::Button(operationRects[0].stretched(-1), U"Add"))
        {
            transformList.push_back(SerializeTransform{
                "Empty",
                {0, 0, 0},
                {0, 0, 0},
                {1, 1, 1}
            });
        }

        // Shape2D_Text::MPlus1_24_Bitmap(U"デバッグ機能 ABC abc 012")
        //     .setPosition(Float2{10, 100})
        //     .setSize(100)
        //     .pushAuto();

        ShapeDrawer::Global().draw();
    }

    // -----------------------------------------------

    const std::string transformsFilepath = "asset/edit/transforms.toml";

    Float3 parse_vec3(const toml::array& arr, Float3 def = {0, 0, 0})
    {
        Float3 f = def;
        if (arr.size() >= 3)
        {
            f.x = arr[0].value_or(def.x);
            f.y = arr[1].value_or(def.y);
            f.z = arr[2].value_or(def.z);
        }
        return f;
    }

    SerializeTransform from_toml(const toml::table& tbl)
    {
        SerializeTransform st;
        st.tag = tbl["tag"].value_or<std::string>("");

        if (auto* arr = tbl["transform"].as_array())
        {
            if (arr->size() >= 3)
            {
                if (auto* pos = (*arr)[0].as_array())
                    st.position = parse_vec3(*pos, {0, 0, 0});
                if (auto* rot = (*arr)[1].as_array())
                    st.rotation = parse_vec3(*rot, {0, 0, 0});
                if (auto* scale = (*arr)[2].as_array())
                    st.scale = parse_vec3(*scale, {1, 1, 1});
            }
        }
        return st;
    }

    Array<SerializeTransform> loadTransformList()
    {
        Array<SerializeTransform> result;

        try
        {
            auto tbl = toml::parse_file(transformsFilepath);

            if (auto* arr = tbl["transforms"].as_array())
            {
                for (auto&& node : *arr)
                {
                    if (auto* t = node.as_table())
                        result.push_back(from_toml(*t));
                }
            }
        }
        catch (const toml::parse_error& err)
        {
            std::cerr << "TOML parse error: " << err.description() << " at " << err.source().begin << "\n";
        }

        return result;
    }

    toml::table to_toml(const SerializeTransform& st)
    {
        return toml::table{
            {"tag", st.tag},
            {
                "transform",
                toml::array{
                    toml::array{st.position.x, st.position.y, st.position.z},
                    toml::array{st.rotation.x, st.rotation.y, st.rotation.z},
                    toml::array{st.scale.x, st.scale.y, st.scale.z},
                },
            }
        };
    }

    void saveTransformList(const Array<SerializeTransform>& transformList)
    {
        toml::array transforms;
        for (auto& e : transformList)
        {
            transforms.push_back(to_toml(e));
        }

        toml::table root{
            {"transforms", transforms}
        };

        std::ofstream file(transformsFilepath);
        file << root;
    }
}

struct DebugEditor::Impl : ActorBase
{
    ActorContainer m_children{};

    Array<SerializeTransform> m_transformList{};

    void init()
    {
        m_transformList = loadTransformList();
    }

    void update() override
    {
        m_children.updateEach();

        transformListEditor(m_transformList);
    }

    void draw() const override
    {
        m_children.drawEach();
    }

    void killed() override
    {
        m_children.killEach();

        saveTransformList(m_transformList);
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
