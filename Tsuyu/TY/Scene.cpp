#include "pch.h"
#include "Scene.h"

#include "detail/EngineCore.h"
#include "detail/EngineRenderContext.h"

namespace TY
{
    using namespace detail;

    void Scene::RequestResize(TY::Size size)
    {
        EngineRenderContext::RequestFrameBufferSize(size);
    }

    Size Scene::Size()
    {
        return EngineRenderContext::FrameBufferSize();
    }

    Point Scene::Center()
    {
        return Size() / 2;
    }

    TY::Rect Scene::Rect()
    {
        return TY::Rect{Point::Zero(), Size()};
    }
}
