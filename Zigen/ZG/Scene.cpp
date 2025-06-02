#include "pch.h"
#include "Scene.h"

#include "detail/EngineCore.h"

namespace ZG
{
    using namespace detail;

    Size Scene::Size()
    {
        return EngineCore.GetSceneSize();
    }

    Point Scene::Center()
    {
        return Size() / 2;
    }
}
