#pragma once

namespace TY
{
    class IComponent
    {
    public:
        virtual ~IComponent() = default;

        virtual bool init() { return true; }

        virtual bool update() { return true; }

        virtual void beforeFlush() { return; }

        virtual void afterPresent() { return; }
    };
}
