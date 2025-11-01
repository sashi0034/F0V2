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

    TY::SizeF Scene::SizeF()
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

    TY::RectF Scene::RectF()
    {
        return TY::RectF{Float2::Zero(), Size()};
    }

    Point Scene::TopLeft()
    {
        return Rect().getRelativePoint(Alignment9::TopLeft);
    }

    Point Scene::TopCenter()
    {
        return Rect().getRelativePoint(Alignment9::TopCenter);
    }

    Point Scene::TopRight()
    {
        return Rect().getRelativePoint(Alignment9::TopRight);
    }

    Point Scene::MiddleLeft()
    {
        return Rect().getRelativePoint(Alignment9::MiddleLeft);
    }

    Point Scene::MiddleCenter()
    {
        return Rect().getRelativePoint(Alignment9::MiddleCenter);
    }

    Point Scene::MiddleRight()
    {
        return Rect().getRelativePoint(Alignment9::MiddleRight);
    }

    Point Scene::BottomLeft()
    {
        return Rect().getRelativePoint(Alignment9::BottomLeft);
    }

    Point Scene::BottomCenter()
    {
        return Rect().getRelativePoint(Alignment9::BottomCenter);
    }

    Point Scene::BottomRight()
    {
        return Rect().getRelativePoint(Alignment9::BottomRight);
    }

    Float2 Scene::TopLeftF()
    {
        return RectF().getRelativePoint(Alignment9::TopLeft);
    }

    Float2 Scene::TopCenterF()
    {
        return RectF().getRelativePoint(Alignment9::TopCenter);
    }

    Float2 Scene::TopRightF()
    {
        return RectF().getRelativePoint(Alignment9::TopRight);
    }

    Float2 Scene::MiddleLeftF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleLeft);
    }

    Float2 Scene::MiddleCenterF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleCenter);
    }

    Float2 Scene::MiddleRightF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleRight);
    }

    Float2 Scene::BottomLeftF()
    {
        return RectF().getRelativePoint(Alignment9::BottomLeft);
    }

    Float2 Scene::BottomCenterF()
    {
        return RectF().getRelativePoint(Alignment9::BottomCenter);
    }

    Float2 Scene::BottomRightF()
    {
        return RectF().getRelativePoint(Alignment9::BottomRight);
    }
}
