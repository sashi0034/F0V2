#include "pch.h"
#include "Scene.h"

#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"

namespace TY
{
    using namespace detail;

    Size Scene::Size()
    {
        return EngineRenderContext::FrameBufferSize();
    }

    Point Scene::Center()
    {
        return Size() / 2;
    }
}
