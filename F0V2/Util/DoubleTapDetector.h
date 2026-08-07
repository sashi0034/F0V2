#pragma once
#include "TY/System.h"

namespace Util
{
    class DoubleTapDetector
    {
    public:
        explicit DoubleTapDetector(float interval = 0.25f);

        void setInterval(float interval)
        {
            m_interval = interval;
        }

        float getInterval() const
        {
            return m_interval;
        }

        void setRemainingTime(float remainingTime)
        {
            m_remainingTime = remainingTime;
        }

        float getRemainingTime() const
        {
            return m_remainingTime;
        }

        template <class Input>
        bool update(const Input& input, float deltaTime = System::DeltaTime())
        {
            if constexpr (requires { input.pressed(); })
            {
                return update(input.pressed(), deltaTime);
            }
            else
            {
                return update(input.pressed, deltaTime);
            }
        }

        bool update(bool pressed, float deltaTime = System::DeltaTime());

        void reset();

    private:
        float m_interval;
        float m_remainingTime{};
        bool m_wasPressed{};
    };
}
