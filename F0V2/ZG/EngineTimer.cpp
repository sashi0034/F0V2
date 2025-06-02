#include "pch.h"
#include "EngineTimer.h"

using namespace ZG;
using namespace ZG::detail;

namespace
{
    class Impl
    {
    public:
        Impl() { Reset(); }

        void Reset()
        {
            m_startTime = clock::now();
            m_lastFrameTime = m_startTime;
        }

        void Tick()
        {
            auto now = clock::now();
            m_deltaTime = std::chrono::duration<double>(now - m_lastFrameTime).count();
            m_lastFrameTime = now;

            m_frameCount++;
        }

        double GetDeltaTime() const { return m_deltaTime; }

        double GetElapsedTime() const
        {
            return std::chrono::duration<double>(clock::now() - m_startTime).count();
        }

        uint64_t GetFrameCount() const { return m_frameCount; }

    private:
        using clock = std::chrono::high_resolution_clock;
        clock::time_point m_startTime;
        clock::time_point m_lastFrameTime;
        double m_deltaTime = 0.0f;
        uint64_t m_frameCount = 0;
    } s_engineTimer;
}

namespace ZG
{
    void EngineTimer_impl::Reset() const
    {
        s_engineTimer.Reset();
    }

    void EngineTimer_impl::Tick() const
    {
        s_engineTimer.Tick();
    }

    double EngineTimer_impl::GetDeltaTime() const
    {
        return s_engineTimer.GetDeltaTime();
    }

    uint64_t EngineTimer_impl::GetFrameCount() const
    {
        return s_engineTimer.GetFrameCount();
    }
}
