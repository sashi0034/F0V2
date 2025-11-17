#pragma once
#include <memory>

namespace TY
{
    class Audio
    {
    public:
        Audio() = default;

        Audio(std::string_view path);

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
