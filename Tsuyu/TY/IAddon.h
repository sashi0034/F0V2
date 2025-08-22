#pragma once
#include "IComponent.h"

namespace TY
{
    class IAddon : public IComponent
    {
    public:
        void beforeFlush() override
        {
            draw();
        }

        // Siv3D compatibility
        virtual void draw() { return; }

        void afterPresent() override
        {
            postPresent();
        }

        // Siv3D compatibility
        virtual void postPresent() { return; }
    };
}
