#include "pch.h"
#include "DebugTomlValue.h"

#include "TY/Addon.h"
#include "TY/FileWatcher.h"
#include "TY/IAddon.h"
#include "TY/Logger.h"
#include "TY/System.h"

namespace
{
    constexpr std::string_view debugTomlPath = "asset/debug.toml";
    constexpr std::string_view debugTomlPathExample = "asset/debug.example.toml";

    toml::parse_result s_parseResult{};

    struct DebugTomlValueAddon : IAddon
    {
        FileWatcher m_watcher{debugTomlPath.data()};

        bool init() override
        {
            if (not std::filesystem::exists(debugTomlPath))
            {
                if (not std::filesystem::exists(debugTomlPathExample))
                {
                    System::ModalError(std::format(
                        "DebugTomlValueAddon::init(): {} is missing.", debugTomlPathExample));
                    return false;
                }
                else
                {
                    std::filesystem::copy(debugTomlPathExample, debugTomlPath);
                }
            }

            reload();
            return true;
        }

        void reload()
        {
            try
            {
                auto result = toml::parse_file(debugTomlPath);
                s_parseResult = std::move(result);
            }
            catch (const toml::parse_error& err)
            {
                const auto& source = err.source();
                LogError("DebugTomlValueAddon::reload(): {} at {}:{}",
                         err.description(), source.begin.line, source.end.column);
            }
        }

        bool update() override
        {
            if (m_watcher.wasChangedThisFrame())
            {
                reload();
            }

            return true;
        }
    };
}

namespace Util_inline
{
    void InitDebugTomlValueAddon()
    {
        Addon::Register<DebugTomlValueAddon>("DebugTomlValueAddon");
    }

    toml::node_view<toml::node> GetDebugTomlValueInternal()
    {
        assert(not s_parseResult.empty());
        return s_parseResult["debug"];
    }
}
