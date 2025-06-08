#include "pch.h"
#include "Scene.h"

#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"

namespace TY
{
    using namespace detail;

    Size Scene::Size()
    {
        return EngineRenderContext::GetSceneSize();
    }

    Point Scene::Center()
    {
        return Size() / 2;
    }
}
