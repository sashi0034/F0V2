#include "pch.h"
#include "WindowsPlacementAddon.h"

#include "MonitorConfiguration.h"

#include "TY/Addon.h"
#include "TY/IAddon.h"
#include "TY/Logger.h"
#include "TY/Window.h"

namespace
{
    constexpr std::string_view placementFilePath = "save/window.toml";

    struct Placement
    {
        std::string monitorConfiguration;
        Point position{};
    };

    struct WindowsPlacementAddon : IAddon
    {
        Point m_position{};

        std::string m_monitorConfiguration{};
        std::vector<Placement> m_placements{};

        ~WindowsPlacementAddon() override
        {
            save();
        }

        bool init() override
        {
            m_position = Window::GetPosition();
            m_monitorConfiguration = SerializeCurrentMonitorConfiguration();

            if (not std::filesystem::exists(placementFilePath))
            {
                return true;
            }

            try
            {
                const auto table = toml::parse_file(placementFilePath);

                if (const auto* placements = table["placements"].as_array())
                {
                    for (const auto& placementNode : *placements)
                    {
                        const auto* placement = placementNode.as_table();
                        if (not placement)
                        {
                            continue;
                        }

                        const auto monitorConfiguration =
                            (*placement)["monitor_configuration"].value<std::string>();
                        const auto x = (*placement)["x"].value<int>();
                        const auto y = (*placement)["y"].value<int>();
                        if (monitorConfiguration && x && y)
                        {
                            setPlacement(*monitorConfiguration, {*x, *y});
                        }
                    }
                }

                if (const auto position = findPosition(m_monitorConfiguration))
                {
                    m_position = *position;
                    Window::SetPosition(m_position);
                }
                else
                {
                    // Legacy format migration
                    const auto x = table["x"].value<int>();
                    const auto y = table["y"].value<int>();
                    if (x && y)
                    {
                        m_position = {*x, *y};
                        Window::SetPosition(m_position);
                    }
                }
            }
            catch (const toml::parse_error& err)
            {
                LogWarning("WindowsPlacementAddon: Failed to parse '{}': {}",
                           placementFilePath,
                           err.description());
            }

            return true;
        }

        bool update() override
        {
            m_position = Window::GetPosition();
            return true;
        }

        void save()
        {
            try
            {
                m_monitorConfiguration = SerializeCurrentMonitorConfiguration();
                setPlacement(m_monitorConfiguration, m_position);

                const std::filesystem::path path{placementFilePath};
                std::filesystem::create_directories(path.parent_path());

                std::ofstream stream{path};
                if (not stream)
                {
                    LogWarning("WindowsPlacementAddon: Failed to open '{}'.", placementFilePath);
                    return;
                }

                toml::array placements{};
                for (const auto& placement : m_placements)
                {
                    placements.push_back(toml::table{
                        {"monitor_configuration", placement.monitorConfiguration},
                        {"x", placement.position.x},
                        {"y", placement.position.y},
                    });
                }

                toml::table root{};
                root.insert("placements", std::move(placements));
                stream << root;
            }
            catch (const std::exception& err)
            {
                LogWarning("WindowsPlacementAddon: Failed to save '{}': {}",
                           placementFilePath,
                           err.what());
            }
        }

        [[nodiscard]]
        std::optional<Point> findPosition(std::string_view monitorConfiguration) const
        {
            for (const auto& placement : m_placements)
            {
                if (placement.monitorConfiguration == monitorConfiguration)
                {
                    return placement.position;
                }
            }
            return {};
        }

        void setPlacement(std::string_view monitorConfiguration, Point position)
        {
            for (auto& placement : m_placements)
            {
                if (placement.monitorConfiguration == monitorConfiguration)
                {
                    placement.position = position;
                    return;
                }
            }

            m_placements.push_back({std::string{monitorConfiguration}, position});
        }
    };
}

namespace Util_inline
{
    void InitWindowsPlacementAddon()
    {
        Addon::Register<WindowsPlacementAddon>("WindowsPlacementAddon");
    }
}
