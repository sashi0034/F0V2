#include "pch.h"
#include "Screen.h"

#include "detail/EngineCore.h"
#include "detail/RenderContext_singleton.h"

namespace TY
{
    using namespace detail;

    void Screen::RequestResize(TY::Size size)
    {
        RenderContext_singleton::RequestFrameBufferSize(size);
    }

    Size Screen::Size()
    {
        return RenderContext_singleton::FrameBufferSize();
    }

    TY::SizeF Screen::SizeF()
    {
        return RenderContext_singleton::FrameBufferSize();
    }

    Point Screen::Center()
    {
        return Size() / 2;
    }

    TY::Rect Screen::Rect()
    {
        return TY::Rect{Point::Zero(), Size()};
    }

    TY::RectF Screen::RectF()
    {
        return TY::RectF{Float2::Zero(), Size()};
    }

    Point Screen::TopLeft()
    {
        return Rect().getRelativePoint(Alignment9::TopLeft);
    }

    Point Screen::TopCenter()
    {
        return Rect().getRelativePoint(Alignment9::TopCenter);
    }

    Point Screen::TopRight()
    {
        return Rect().getRelativePoint(Alignment9::TopRight);
    }

    Point Screen::MiddleLeft()
    {
        return Rect().getRelativePoint(Alignment9::MiddleLeft);
    }

    Point Screen::MiddleCenter()
    {
        return Rect().getRelativePoint(Alignment9::MiddleCenter);
    }

    Point Screen::MiddleRight()
    {
        return Rect().getRelativePoint(Alignment9::MiddleRight);
    }

    Point Screen::BottomLeft()
    {
        return Rect().getRelativePoint(Alignment9::BottomLeft);
    }

    Point Screen::BottomCenter()
    {
        return Rect().getRelativePoint(Alignment9::BottomCenter);
    }

    Point Screen::BottomRight()
    {
        return Rect().getRelativePoint(Alignment9::BottomRight);
    }

    Float2 Screen::TopLeftF()
    {
        return RectF().getRelativePoint(Alignment9::TopLeft);
    }

    Float2 Screen::TopCenterF()
    {
        return RectF().getRelativePoint(Alignment9::TopCenter);
    }

    Float2 Screen::TopRightF()
    {
        return RectF().getRelativePoint(Alignment9::TopRight);
    }

    Float2 Screen::MiddleLeftF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleLeft);
    }

    Float2 Screen::MiddleCenterF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleCenter);
    }

    Float2 Screen::MiddleRightF()
    {
        return RectF().getRelativePoint(Alignment9::MiddleRight);
    }

    Float2 Screen::BottomLeftF()
    {
        return RectF().getRelativePoint(Alignment9::BottomLeft);
    }

    Float2 Screen::BottomCenterF()
    {
        return RectF().getRelativePoint(Alignment9::BottomCenter);
    }

    Float2 Screen::BottomRightF()
    {
        return RectF().getRelativePoint(Alignment9::BottomRight);
    }
}
