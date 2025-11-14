#include "pch.h"

#include "GameFlowchart.h"
#include "TY_Extension/LivePPAddon.h"
#include "Race/RaceScene.h"
#include "Testbed/Testbed_AirCombat.h"
#include "Testbed/Testbed_Basic3D.h"
#include "Testbed/Testbed_Collision1.h"
#include "Testbed/Testbed_Collision2.h"
#include "Testbed/Testbed_Collision3.h"
#include "Testbed/Testbed_Intersection.h"
#include "Testbed/Testbed_Font.h"
#include "Testbed/Testbed_Ocean.h"
#include "Testbed/Testbed_RenderTarget.h"
#include "Testbed/Testbed_ShadowMap.h"
#include "Testbed/Testbed_ImmediateDrawer.h"
#include "Testbed/Testbed_Shadertoy.h"
#include "Testbed/Testbed_WaveTest.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"

void Main()
{
#if defined(_DEBUG)
    InitLivePPAddon();

    InitDebugTomlValueAddon();
#endif

    InitImmediatePrintAddon();

#if 0
    Testbed_WaveTest();
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
