#pragma once

namespace TY::detail
{
    class IEngineDrawer
    {
    public:
        virtual ~IEngineDrawer() = default;

        virtual void beforeFlush()
        {
        }

        virtual void afterFlush()
        {
        }
    };
}
