#pragma once

namespace GM
{
    struct DebugService
    {
        bool editorEnabled{true};
    };

    inline DebugService g_debugService{};
}
