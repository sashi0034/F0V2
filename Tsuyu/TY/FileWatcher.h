#pragma once
#include "ITimestamp.h"
#include "UnifiedString.h"

namespace TY
{
    class FileWatcher
    {
    public:
        FileWatcher() = default;

        FileWatcher(const UnifiedString& path);

        std::shared_ptr<ITimestamp> timestamp() const;

        bool isChangedInFrame() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl{};
    };
}
