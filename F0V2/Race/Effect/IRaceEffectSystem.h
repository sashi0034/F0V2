#pragma once

#include "Race/IRaceDrawer.h"
#include "TY/Vector3D.h"

namespace Race
{
    struct RaceEffectFrameContext
    {
        float deltaTime{};

        float elapsedTime{};

        Float3 cameraUp{};

        Float3 cameraRight{};
    };

    class IRaceEffectSystem : public IRaceDrawer
    {
    public:
        virtual ~IRaceEffectSystem() = default;

        virtual void onRegistered()
        {
        }

        virtual void update(const RaceEffectFrameContext& context) = 0;

        virtual void onUnregistered()
        {
        }
    };
}
