#pragma once

namespace ZG
{
    namespace System
    {
        void SetTargetFrameRate(std::optional<double> frameRate);

        [[nodiscard]] std::optional<double> TargetFrameRate();

        bool Update();

        double DeltaTime();

        uint64_t FrameCount();

        void ModalError(const std::wstring& message);

        void ModalError(const std::string& message);
    }
}
