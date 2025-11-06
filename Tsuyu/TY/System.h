#pragma once
#include "IGpuMemoryUsage.h"

namespace TY
{
    namespace System
    {
        bool Update();

        float Time();

        float DeltaTime();

        uint64_t FrameCount();

        void Sleep(uint64_t ms);

        void ModalError(const std::wstring& message);

        void ModalError(const std::string& message);
    }
}
