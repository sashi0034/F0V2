#include "pch.h"

#include "GameFlowchart.h"
#include "TY_Extension/LivePPAddon.h"
#include "Race/RaceScene.h"
#include "Testbed/Testbed_AirCombat.h"
#include "Testbed/Testbed_Basic3D.h"
#include "Testbed/Testbed_Font.h"
#include "Testbed/Testbed_RenderTarget.h"
#include "Testbed/Testbed_ShadowMap.h"
#include "Testbed/Testbed_ImmediateDrawer.h"
#include "Testbed/Testbed_Shadertoy.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"
#include "Util/DebugTomlValue.h"
#include "Util/ImmediatePrint.h"
#include "Util/WindowsPlacementAddon.h"

void Main()
{
#if defined(_DEBUG)
    InitLivePPAddon();

    InitDebugTomlValueAddon();

    InitWindowsPlacementAddon();
#endif

    InitImmediatePrintAddon();

#if 0
    Testbed_WaveTest();
#else
    using namespace TY;

    ActorContainer actors{};

    auto flowchart = actors.birth(GameFlowchart());
    flowchart.init();

    while (System::Update())
    {
        actors.updateEach();
    }

    actors.killEach();
#endif
}
