#pragma once

namespace GM
{
    struct DebugService
    {
        bool editorEnabled{true};

        float cameraSpeed{3.0f};
    };

    inline DebugService g_debugService{};
}
