#pragma once

inline namespace GM_inline
{
    struct DebugService
    {
        bool editorEnabled{};

        float cameraSpeed{3.0f};

        int monitorMachineId{};

        bool disablePlayerInput{};

        bool drawScenery{};
    };

    inline DebugService g_debugService{};
}
