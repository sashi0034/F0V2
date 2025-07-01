#include "pch.h"

#include "TY_Extension/LivePPAddon.h"
#include "Combat/CombatScene.h"
#include "Demo/Demo_AirCombat.h"
#include "Demo/Demo_Gpgpu.h"
#include "Demo/Demo_PointLight.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"

void Main()
{
    InitLivePPAddon();

#if 1
    Demo_AirCombat();
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
