#pragma once
#include "ITimestamp.h"

namespace ZG
{
    class FileWatcher
    {
    public:
        FileWatcher() = default;

        FileWatcher(const std::string& path);

        FileWatcher(const std::wstring& path);

        std::shared_ptr<ITimestamp> timestamp() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
