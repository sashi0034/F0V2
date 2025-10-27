#pragma once

namespace GM
{
    struct DebugService
    {
        bool editorEnabled{true};

        float cameraSpeed{3.0f};

        int monitorMachineId{};

        bool disablePlayerInput{};
    };

    inline DebugService g_debugService{};
}
