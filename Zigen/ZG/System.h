#pragma once

namespace ZG
{
    namespace System
    {
        bool Update();

        double DeltaTime();

        uint64_t FrameCount();

        void ModalError(const std::wstring& message);

        void ModalError(const std::string& message);
    }
}
