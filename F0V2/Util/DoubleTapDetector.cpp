#include "pch.h"
#include "DoubleTapDetector.h"

namespace Util
{
    DoubleTapDetector::DoubleTapDetector(float interval)
        : m_interval(std::max(0.0f, interval))
    {
    }

    bool DoubleTapDetector::update(bool pressed, float deltaTime)
    {
        m_remainingTime = std::max(0.0f, m_remainingTime - deltaTime);
        const bool pressedThisFrame = pressed && not m_wasPressed;
        m_wasPressed = pressed;

        if (not pressedThisFrame)
        {
            return false;
        }

        if (m_remainingTime > 0.0f)
        {
            m_remainingTime = 0.0f;
            return true;
        }

        m_remainingTime = m_interval;
        return false;
    }

    void DoubleTapDetector::reset()
    {
        m_remainingTime = 0.0f;
        m_wasPressed = false;
    }
}
