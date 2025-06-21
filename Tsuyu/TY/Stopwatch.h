#pragma once

namespace TY
{
    class Stopwatch
    {
    public:
        using Clock = std::chrono::steady_clock;

        Stopwatch();

        void pause();

        void resume();

        void reset();

        void restart();

        /// @brief Seconds as double
        double sF() const;

        /// @brief Milliseconds as double
        double msF() const;

        std::chrono::duration<double> elapsed() const;

    private:
        std::chrono::time_point<Clock> m_startTime;
        std::chrono::duration<double> m_elapsed;
        bool m_paused;
    };
}
