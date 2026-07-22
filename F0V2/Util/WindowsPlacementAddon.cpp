#include "pch.h"
#include "WindowsPlacementAddon.h"

#include "TY/Addon.h"
#include "TY/IAddon.h"
#include "TY/Logger.h"
#include "TY/Window.h"

namespace
{
    constexpr std::string_view placementFilePath = "save/window.toml";

    struct WindowsPlacementAddon : IAddon
    {
        Point m_position{};

        ~WindowsPlacementAddon() override
        {
            save();
        }

        bool init() override
        {
            m_position = Window::GetPosition();

            if (not std::filesystem::exists(placementFilePath))
            {
                return true;
            }

            try
            {
                const auto table = toml::parse_file(placementFilePath);
                const auto x = table["x"].value<int>();
                const auto y = table["y"].value<int>();

                if (x && y)
                {
                    m_position = {*x, *y};
                    Window::SetPosition(m_position);
                }
                else
                {
                    LogWarning("WindowsPlacementAddon: '{}' is broken.", placementFilePath);
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

        void save() const
        {
            try
            {
                const std::filesystem::path path{placementFilePath};
                std::filesystem::create_directories(path.parent_path());

                std::ofstream stream{path};
                if (not stream)
                {
                    LogWarning("WindowsPlacementAddon: Failed to open '{}'.", placementFilePath);
                    return;
                }

                stream << toml::table{
                    {"x", m_position.x},
                    {"y", m_position.y},
                };
            }
            catch (const std::exception& err)
            {
                LogWarning("WindowsPlacementAddon: Failed to save '{}': {}",
                           placementFilePath,
                           err.what());
            }
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
