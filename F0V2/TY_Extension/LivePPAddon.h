#pragma once

#if defined(_DEBUG)
namespace TY
{
    void InitLivePPAddon();

    bool IsLivePPHotReloaded();

    bool IsLivePPHotReloaded(const char* sourceFilePath);
}
#endif
