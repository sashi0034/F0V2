#include "pch.h"
#include "Stopwatch.h"

namespace TY
{
    Stopwatch::Stopwatch(): m_startTime(Clock::now()), m_elapsed(0), m_paused(false)
    {
    }

    void Stopwatch::pause()
    {
        if (!m_paused)
        {
            m_elapsed += Clock::now() - m_startTime;
            m_paused = true;
        }
    }

    void Stopwatch::resume()
    {
        if (m_paused)
        {
            m_startTime = Clock::now();
            m_paused = false;
        }
    }

    void Stopwatch::reset()
    {
        m_elapsed = std::chrono::duration<double>::zero();
        m_startTime = Clock::now();
        m_paused = false;
    }

    void Stopwatch::restart()
    {
        m_elapsed = std::chrono::duration<double>::zero();
        m_startTime = Clock::now();
        m_paused = false;
    }

    double Stopwatch::sF() const
    {
        return std::chrono::duration<double>(elapsed()).count();
    }

    double Stopwatch::msF() const
    {
        return std::chrono::duration<double, std::milli>(elapsed()).count();
    }

    std::chrono::duration<double> Stopwatch::elapsed() const
    {
        if (m_paused)
        {
            return m_elapsed;
        }
        else
        {
            return m_elapsed + (Clock::now() - m_startTime);
        }
    }
}
