#include "pch.h"

#include "Combat/CombatScene.h"
#include "TY/System.h"
#include "TY/ActorContainer.h"

void Main()
{
#if 0
    Demo_PointLight();
#else
    using namespace TY;

    ActorContainer actors{};
    actors.birth(Combat::CombatScene());

    while (System::Update())
    {
        actors.updateEach();
    }
#endif
}
