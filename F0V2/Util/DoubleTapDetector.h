#pragma once
#include "TY/System.h"

namespace Util
{
    class DoubleTapDetector
    {
    public:
        explicit DoubleTapDetector(float interval = 0.25f);

        void setInterval(float interval);

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
