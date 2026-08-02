#include "pch.h"
#include "LivePPAddon.h"

#include <cwchar>

#if defined(_DEBUG)

// include the API for Windows, 64-bit, C++
#include "../LivePP/API/x64/LPP_API_x64_CPP.h"
#include "../LivePP/API/x64/LPP_API_Hooks.h"
#include "TY/Addon.h"
#include "TY/Window.h"

using namespace TY;

namespace
{
    bool s_hotReloaded{};
    std::vector<std::wstring> s_hotReloadedFileNames{};

    void OnHotReloadPostPatch(
        lpp::LppHotReloadPostpatchHookId,
        const wchar_t* const,
        const wchar_t* const* const modifiedFiles,
        unsigned int modifiedFilesCount,
        const wchar_t* const* const,
        unsigned int)
    {
        for (unsigned int i = 0; i < modifiedFilesCount; ++i)
        {
            if (modifiedFiles[i] == nullptr) continue;

            s_hotReloadedFileNames.push_back(std::filesystem::path{modifiedFiles[i]}.filename().wstring());
        }
    }

    LPP_HOTRELOAD_POSTPATCH_HOOK(OnHotReloadPostPatch);

    struct LivePPAddon : IAddon
    {
        bool m_initialized{};
        lpp::LppSynchronizedAgent m_lppAgent;

        bool init() override
        {
            // create a synchronized agent, loading the Live++ agent from the given path, e.g. "ThirdParty/LivePP""
            m_lppAgent = lpp::LppCreateSynchronizedAgent(nullptr, L"../LivePP");

            // bail out in case the agent is not valid
            if (not lpp::LppIsValidSynchronizedAgent(&m_lppAgent))
            {
                std::cerr << "Failed to create synchronized agent" << std::endl;
                return true;
            }

            // enable Live++ for all loaded modules
            m_lppAgent.EnableModule(
                lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);

            m_initialized = true;
            return true;
        }

        void postPresent() override
        {
            if (not m_initialized) return;

            s_hotReloaded = false;
            s_hotReloadedFileNames.clear();

            // listen to hot-reload and hot-restart requests
            if (m_lppAgent.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD))
            {
                // client code can do whatever it wants here, e.g. synchronize across several threads, the network, etc.
                // ...
                m_lppAgent.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);

                Window::SetForeground();

                s_hotReloaded = true;
            }

            if (m_lppAgent.WantsRestart())
            {
                // client code can do whatever it wants here, e.g. finish logging, abandon threads, etc.
                // ...
                m_lppAgent.Restart(lpp::LPP_RESTART_BEHAVIOUR_INSTANT_TERMINATION, 0u, nullptr);
            }
        }

        ~LivePPAddon()
        {
            if (not m_initialized) return;

            // destroy the Live++ agent
            lpp::LppDestroySynchronizedAgent(&m_lppAgent);
        }
    } s_livePP{};
}

namespace TY
{
    void InitLivePPAddon()
    {
        Addon::Register<LivePPAddon>("LivePPAddon");
    }

    bool IsLivePPHotReloaded()
    {
        return s_hotReloaded;
    }

    bool IsLivePPHotReloaded(const char* sourceFilePath)
    {
        if (not s_hotReloaded || sourceFilePath == nullptr) return false;

        const auto fileName = std::filesystem::path{sourceFilePath}.filename().wstring();
        if (fileName.empty()) return false;

        return std::ranges::any_of(s_hotReloadedFileNames, [&](const std::wstring& hotReloadedFileName)
        {
            return _wcsicmp(fileName.c_str(), hotReloadedFileName.c_str()) == 0;
        });
    }
}

#endif
