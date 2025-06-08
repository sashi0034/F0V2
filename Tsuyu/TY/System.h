#pragma once

namespace TY
{
    namespace System
    {
        bool Update();

        double DeltaTime();

        uint64_t FrameCount();

        void Sleep(uint64_t ms);

        void ModalError(const std::wstring& message);

        void ModalError(const std::string& message);
    }
}
