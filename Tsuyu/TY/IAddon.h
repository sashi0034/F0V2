#pragma once

namespace TY
{
    class IAddon
    {
    public:
        virtual ~IAddon() = default;

        virtual bool init() { return true; }

        virtual bool update() { return true; }

        virtual void postPresent() { return; }
    };
}
