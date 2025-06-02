#include "pch.h"
#include "LivePPAddon.h"

#if _DEBUG

// include the API for Windows, 64-bit, C++
#include "../LivePP/API/x64/LPP_API_x64_CPP.h"

namespace
{
    bool s_hotReloaded{};

    struct LivePPAddon
    {
        bool m_firstFrame{true};
        bool m_initialized{};
        lpp::LppSynchronizedAgent m_lppAgent;

        bool init()
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

        void update()
        {
            if (m_firstFrame)
            {
                m_firstFrame = false;
                init();
            }

            if (not m_initialized) return;
            // -----------------------------------------------

            s_hotReloaded = false;

            // listen to hot-reload and hot-restart requests
            if (m_lppAgent.WantsReload(lpp::LPP_RELOAD_OPTION_SYNCHRONIZE_WITH_RELOAD))
            {
                // client code can do whatever it wants here, e.g. synchronize across several threads, the network, etc.
                // ...
                m_lppAgent.Reload(lpp::LPP_RELOAD_BEHAVIOUR_WAIT_UNTIL_CHANGES_ARE_APPLIED);

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

namespace Util
{
    void AdvanceLivePP()
    {
        s_livePP.update();
    }

    bool IsLivePPHotReloaded()
    {
        return s_hotReloaded;
    }
}

#endif
