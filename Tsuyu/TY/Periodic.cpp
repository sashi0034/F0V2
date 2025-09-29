#include "pch.h"
#include "Periodic.h"

#include "Math.h"

namespace TY
{
    namespace Periodic
    {
        float Sine0_1(const float periodSec, const float t) noexcept
        {
            const float x = (std::fmod(t, periodSec) / (periodSec * Math::InvTwoPiF));

            return (std::sin(x) * 0.5 + 0.5);
        }

        float Sine0_1(const Duration& period, const float t) noexcept
        {
            return Sine0_1(period.count(), t);
        }

        float Square0_1(const float periodSec, const float t) noexcept
        {
            return (std::fmod(t, periodSec) < (periodSec * 0.5)) ? 1.0 : 0.0;
        }

        float Square0_1(const Duration& period, const float t) noexcept
        {
            return Square0_1(period.count(), t);
        }

        float Pulse0_1(const float periodSec, const float dutyCycle, const float t) noexcept
        {
            return (std::fmod(t, periodSec) < (periodSec * dutyCycle)) ? 1.0 : 0.0;
        }

        float Pulse0_1(const Duration& period, const float dutyCycle, const float t) noexcept
        {
            return Pulse0_1(period.count(), dutyCycle, t);
        }

        float Triangle0_1(const float periodSec, const float t) noexcept
        {
            const float x = (std::fmod(t, periodSec) / (periodSec * 0.5));

            if (x <= 1.0)
            {
                return x;
            }
            else
            {
                return (2.0 - x);
            }
        }

        float Triangle0_1(const Duration& period, const float t) noexcept
        {
            return Triangle0_1(period.count(), t);
        }

        float Sawtooth0_1(const float periodSec, const float t) noexcept
        {
            return std::fmod(t, periodSec) / periodSec;
        }

        float Sawtooth0_1(const Duration& period, const float t) noexcept
        {
            return Sawtooth0_1(period.count(), t);
        }

        float Jump0_1(const float periodSec, const float t) noexcept
        {
            float x = (std::fmod(t, periodSec) / (periodSec * 0.5));

            if (1.0 < x)
            {
                x = (2.0 - x);
            }

            return (2 * x - (x * x));
        }

        float Jump0_1(const Duration& period, const float t) noexcept
        {
            return Jump0_1(period.count(), t);
        }

        float Sine1_1(const float periodSec, const float t) noexcept
        {
            const float x = (std::fmod(t, periodSec) / (periodSec * Math::InvTwoPiF));

            return std::sin(x);
        }

        float Sine1_1(const Duration& period, const float t) noexcept
        {
            return Sine1_1(period.count(), t);
        }

        float Square1_1(const float periodSec, const float t) noexcept
        {
            return (std::fmod(t, periodSec) < (periodSec * 0.5)) ? 1.0 : -1.0;
        }

        float Square1_1(const Duration& period, const float t) noexcept
        {
            return Square1_1(period.count(), t);
        }

        float Pulse1_1(const float periodSec, const float dutyCycle, const float t) noexcept
        {
            return (std::fmod(t, periodSec) < (periodSec * dutyCycle)) ? 1.0 : -1.0;
        }

        float Pulse1_1(const Duration& period, const float dutyCycle, const float t) noexcept
        {
            return Pulse1_1(period.count(), dutyCycle, t);
        }

        float Triangle1_1(const float periodSec, const float t) noexcept
        {
            const float x = (std::fmod(t, periodSec) / (periodSec * 0.5));

            if (x <= 1.0)
            {
                return (2.0 * x - 1.0);
            }
            else
            {
                return (3.0 - 2.0 * x);
            }
        }

        float Triangle1_1(const Duration& period, const float t) noexcept
        {
            return Triangle1_1(period.count(), t);
        }

        float Sawtooth1_1(const float periodSec, const float t) noexcept
        {
            return ((std::fmod(t, periodSec) / periodSec) * 2.0 - 1.0);
        }

        float Sawtooth1_1(const Duration& period, const float t) noexcept
        {
            return Sawtooth1_1(period.count(), t);
        }

        float Jump1_1(const float periodSec, const float t) noexcept
        {
            float x = (std::fmod(t, periodSec) / (periodSec * 0.5));

            if (1.0 < x)
            {
                x = (2.0 - x);
            }

            return ((2 * x - (x * x)) * 2.0 - 1.0);
        }

        float Jump1_1(const Duration& period, const float t) noexcept
        {
            return Jump1_1(period.count(), t);
        }
    }
}
