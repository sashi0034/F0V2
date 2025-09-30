#include "pch.h"
#include "ToyCinnamon.h"

#include "Asset.generated.h"
#include "TY/ActorContainer.h"
#include "TY/ModelDrawer.h"
#include "TY_Extension/GameObjectBase.h"
#include "TY_Extension/SerializeTransform.h"

using namespace Race;

namespace
{
    bool imguiFloat3(const char* label, Float3& v, float columnWidth = 100.0f)
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
        // char buffer[256];
        // strcpy_s(buffer, tr.tag.c_str());
        // ImGui::Text("Tag:");
        // ImGui::SameLine();
        // if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
        // {
        //     tr.tag = buffer;
        // }
        //
        // ImGui::Separator();

        // 各プロパティ
        if (imguiFloat3("Position", tr.position, 80.0f))
        {
        }

        if (imguiFloat3("Rotation", tr.rotation, 80.0f))
        {
        }

        if (imguiFloat3("Scale", tr.scale, 80.0f))
        {
        }

        ImGui::End();
    }

    // -----------------------------------------------

    const std::string transformsFilepath = "asset/edit/ToyCinnamon.toml";

    SerializeTransform loadTransform()
    {
        SerializeTransform result;

        try
        {
            auto tbl = toml::parse_file(transformsFilepath);
            result = SerializeTransform::Deserialize(*tbl.as_table());
        }
        catch (const toml::parse_error& err)
        {
            std::cerr << "TOML parse error: " << err.description() << " at " << err.source().begin << "\n";
        }

        return result;
    }

    void saveTransform(const SerializeTransform& transform)
    {
        toml::table root = transform.serialize();
        std::ofstream file(transformsFilepath);
        file << root;
    }
}

struct ToyCinnamon::Impl : GameObjectBase
{
    ActorContainer m_children{};

    ModelDrawer m_drawer{};

    SerializeTransform m_transform{};

    void Init()
    {
        m_drawer =
            ModelDrawerParams{}
            .setModel(Asset_model::cinnamon)
            .setShader(Asset_shader::model);

        m_transform = loadTransform();
    }

private:
    void update() override
    {
        m_drawer.uploadWorldMatrix(m_transform.getMatrix()).draw();
    }

    void killed() override
    {
        m_children.killEach();

        saveTransform(m_transform);
    }

    std::u32string name() const override
    {
        return U"ToyCinnamon";
    }

    void debugInspector() override
    {
        imguiTransformInspector(m_transform);
    }
};

namespace Race
{
    ToyCinnamon::ToyCinnamon() :
        p_impl(std::make_shared<Impl>())
    {
    }

    void ToyCinnamon::init()
    {
        p_impl->Init();
        GameObjectHandle::init();
    }

    std::shared_ptr<GameObjectBase> ToyCinnamon::asGameObject() const
    {
        return p_impl;
    }
}
