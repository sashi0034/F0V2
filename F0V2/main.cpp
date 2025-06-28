#include "pch.h"

#include "TY_Extension/LivePPAddon.h"
#include "Combat/CombatScene.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"

void Main()
{
    InitLivePPAddon();

#if 0
    Demo_PointLight();
#else
    using namespace TY;

    ActorContainer actors{};

    auto combat = actors.birth(Combat::CombatScene());
    combat.init();

    while (System::Update())
    {
        actors.updateEach();
    }
#endif
}
