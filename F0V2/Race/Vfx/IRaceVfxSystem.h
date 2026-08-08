#pragma once

#include "Race/IRaceDrawer.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct RaceVfxFrameContext
    {
        float deltaTime{};

        float elapsedTime{};

        Float3 cameraUp{};

        Float3 cameraRight{};
    };

    class IRaceVfxSystem : public IRaceDrawer
    {
    public:
        virtual ~IRaceVfxSystem() = default;

        virtual void onRegistered()
        {
        }

        virtual void update(const RaceVfxFrameContext& context) = 0;

        virtual void onUnregistered()
        {
        }
    };
}
