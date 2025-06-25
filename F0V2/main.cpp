#include "pch.h"

#include "Combat/CombatScene.h"
#include "TY/System.h"
#include "Util/ActorContainer.h"

void Main()
{
#if 0
    Demo_PointLight();
#else
    using namespace Util;

    ActorContainer actors{};
    actors.Birth(Combat::CombatScene());

    while (System::Update())
    {
        actors.UpdateEach();
    }
#endif
}
