#include "pch.h"

#include "GameFlowchart.h"
#include "TY_Extension/LivePPAddon.h"
#include "Race/RaceScene.h"
#include "Demo/Demo_AirRace.h"
#include "Demo/Demo_Basic3D.h"
#include "Demo/Demo_Collision1.h"
#include "Demo/Demo_Collision2.h"
#include "Demo/Demo_Collision3.h"
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

#if 0
    Demo_Collision3();
#else
    using namespace TY;

    ActorContainer actors{};

    actors.birth(F0V2::GameFlowchart());

    while (System::Update())
    {
        actors.updateEach();
    }

    actors.killEach();
#endif
}
