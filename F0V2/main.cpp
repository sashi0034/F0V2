#include "pch.h"

#include "TY_Extension/LivePPAddon.h"
#include "Combat/CombatScene.h"
#include "Demo/Demo_AirCombat.h"
#include "Demo/Demo_Basic3D.h"
#include "Demo/Demo_Collision.h"
#include "Demo/Demo_Intersection.h"
#include "Demo/Demo_Font.h"
#include "Demo/Demo_Gpgpu.h"
#include "Demo/Demo_Ocean.h"
#include "Demo/Demo_RenderTarget.h"
#include "Demo/Demo_ShadowMap.h"
#include "Demo/Demo_ShapeDrawer.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"

void Main()
{
    InitLivePPAddon();

#if 1
    Demo_Collision();
#else
    using namespace TY;

    ActorContainer actors{};

    auto combat = actors.birth(Combat::CombatScene());
    combat.init();

    while (System::Update())
    {
        actors.updateEach();
    }

    actors.killEach();
#endif
}
