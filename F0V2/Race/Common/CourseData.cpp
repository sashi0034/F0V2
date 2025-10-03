#include "pch.h"
#include "CourseData.h"

namespace Race
{
    float CourseNode::rollRadians() const
    {
        return Math::ToRadians(static_cast<float>(roll));
    }

    CourseData LoadCourseData(const std::string& filepath)
    {
        CourseData result;

        try
        {
            auto tbl = toml::parse_file(filepath);

            if (auto* arr = tbl["nodes"].as_array())
            {
                for (auto&& nodeVal : *arr)
                {
                    if (auto* nodeTbl = nodeVal.as_table())
                    {
                        CourseNode node{};

                        if (auto* posArr = (*nodeTbl)["pos"].as_array())
                        {
                            if (posArr->size() == 3)
                            {
                                node.pos.x = static_cast<float>((*posArr)[0].value_or(0.0));
                                node.pos.y = static_cast<float>((*posArr)[1].value_or(0.0));
                                node.pos.z = static_cast<float>((*posArr)[2].value_or(0.0));
                            }
                        }

                        node.roll = static_cast<float>((*nodeTbl)["roll"].value_or(0.0));

                        result.nodes.push_back(node);
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

    void SaveCourseData(const CourseData& course, const std::string& filepath)
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
            nodeTbl.insert("roll", node.roll);

            nodesArr.push_back(std::move(nodeTbl));
        }

        root.insert("nodes", std::move(nodesArr));

        std::ofstream file(filepath);
        file << root;
    }
}
