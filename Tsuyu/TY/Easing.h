#pragma once
#include "Math.h"

//
// TERMS OF USE - EASING EQUATIONS
//
// Open source under the BSD License. 
//
// Copyright (c) 2001 Robert Penner
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
//   copyright notice, this list of conditions and the following disclaimer
//   in the documentation and/or other materials provided with the
//   distribution.
// * Neither the name of the author nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

namespace TY
{
    namespace detail
    {
        inline double ElasticAP(double t, double a, const double p) noexcept
        {
            if (t == 0)
            {
                return 0.0;
            }
            else if (t == 1)
            {
                return 1.0;
            }

            double s;

            if (a < 1.0)
            {
                a = 1.0;

                s = p / 4.0f;
            }
            else
            {
                s = p * Math::InvTwoPi * std::asin(1.0 / a);
            }

            t -= 1.0;

            return -(a * std::exp2(10.0 * t) * std::sin((t - s) * Math::TwoPi / p));
        }
    }

    namespace Easing
    {
        [[nodiscard]]
        constexpr double Linear(const double t) noexcept
        {
            return t;
        }

        [[nodiscard]]
        inline double Sine(const double t) noexcept
        {
            return (1.0 - std::cos(t * Math::HalfPi));
        }

        [[nodiscard]]
        constexpr double Quad(const double t) noexcept
        {
            return (t * t);
        }

        [[nodiscard]]
        constexpr double Cubic(const double t) noexcept
        {
            return (t * t * t);
        }

        [[nodiscard]]
        constexpr double Quart(const double t) noexcept
        {
            return (t * t) * (t * t);
        }

        [[nodiscard]]
        constexpr double Quint(const double t) noexcept
        {
            return (t * t) * (t * t) * t;
        }

        [[nodiscard]]
        inline double Expo(const double t) noexcept
        {
            if (t == 0.0)
            {
                return 0.0;
            }
            else
            {
                return std::exp2(10.0 * (t - 1.0));
            }
        }

        [[nodiscard]]
        inline double Circ(const double t) noexcept
        {
            return (1.0 - std::sqrt(1.0 - t * t));
        }

        [[nodiscard]]
        constexpr double Back(const double t) noexcept
        {
            return t * t * (2.70158 * t - 1.70158);
        }

        [[nodiscard]]
        inline double Elastic(const double t) noexcept
        {
            return detail::ElasticAP(t, 1.0, 0.3);
        }

        [[nodiscard]]
        constexpr double Bounce(double t) noexcept
        {
            t = (1.0 - t);

            if (t < (1 / 2.75))
            {
                return (1.0 - (7.5625 * t * t));
            }
            else if (t < (2 / 2.75))
            {
                t -= 1.5 / 2.75;
                return (1.0 - (7.5625 * t * t + 0.75));
            }
            else if (t < (2.5 / 2.75))
            {
                t -= 2.25 / 2.75;
                return (1.0 - (7.5625 * t * t + 0.9375));
            }
            else
            {
                t -= (2.625 / 2.75);
                return (1.0 - (7.5625 * t * t + 0.984375));
            }
        }
    }

    [[nodiscard]]
    constexpr double EaseIn(double f(double), const double t) noexcept
    {
        return f(t);
    }

    template <class Type>
    [[nodiscard]]
    constexpr auto EaseIn(double f(double), const Type& start, const Type& end, const double t)
    {
        return Math::Lerp(start, end, f(t));
    }

    [[nodiscard]]
    constexpr double EaseOut(double f(double), const double t) noexcept
    {
        return (1.0 - f(1.0 - t));
    }

    template <class Type>
    [[nodiscard]]
    constexpr auto EaseOut(double f(double), const Type& start, const Type& end, const double t)
    {
        return Math::Lerp(start, end, EaseOut(f, t));
    }

    [[nodiscard]]
    constexpr double EaseInOut(double f(double), const double t) noexcept
    {
        return ((t < 0.5) ? (f(2.0 * t) * 0.5) : (0.5 + EaseOut(f, 2.0 * t - 1.0) * 0.5));
    }

    template <class Type>
    [[nodiscard]]
    constexpr auto EaseInOut(double f(double), const Type& start, const Type& end, const double t)
    {
        return Math::Lerp(start, end, EaseInOut(f, t));
    }

    [[nodiscard]]
    constexpr double EaseInLinear(const double t) noexcept
    {
        return Easing::Linear(t);
    }

    [[nodiscard]]
    constexpr double EaseOutLinear(const double t) noexcept
    {
        return EaseOut(Easing::Linear, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutLinear(const double t) noexcept
    {
        return EaseInOut(Easing::Linear, t);
    }

    [[nodiscard]]
    inline double EaseInSine(const double t) noexcept
    {
        return Easing::Sine(t);
    }

    [[nodiscard]]
    inline double EaseOutSine(const double t) noexcept
    {
        return EaseOut(Easing::Sine, t);
    }

    [[nodiscard]]
    inline double EaseInOutSine(const double t) noexcept
    {
        return EaseInOut(Easing::Sine, t);
    }

    [[nodiscard]]
    constexpr double EaseInQuad(const double t) noexcept
    {
        return Easing::Quad(t);
    }

    [[nodiscard]]
    constexpr double EaseOutQuad(const double t) noexcept
    {
        return EaseOut(Easing::Quad, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutQuad(const double t) noexcept
    {
        return EaseInOut(Easing::Quad, t);
    }

    [[nodiscard]]
    constexpr double EaseInCubic(const double t) noexcept
    {
        return Easing::Cubic(t);
    }

    [[nodiscard]]
    constexpr double EaseOutCubic(const double t) noexcept
    {
        return EaseOut(Easing::Cubic, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutCubic(const double t) noexcept
    {
        return EaseInOut(Easing::Cubic, t);
    }

    [[nodiscard]]
    constexpr double EaseInQuart(const double t) noexcept
    {
        return Easing::Quart(t);
    }

    [[nodiscard]]
    constexpr double EaseOutQuart(const double t) noexcept
    {
        return EaseOut(Easing::Quart, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutQuart(const double t) noexcept
    {
        return EaseInOut(Easing::Quart, t);
    }

    [[nodiscard]]
    constexpr double EaseInQuint(const double t) noexcept
    {
        return Easing::Quint(t);
    }

    [[nodiscard]]
    constexpr double EaseOutQuint(const double t) noexcept
    {
        return EaseOut(Easing::Quint, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutQuint(const double t) noexcept
    {
        return EaseInOut(Easing::Quint, t);
    }

    [[nodiscard]]
    inline double EaseInExpo(const double t) noexcept
    {
        return Easing::Expo(t);
    }

    [[nodiscard]]
    inline double EaseOutExpo(const double t) noexcept
    {
        return EaseOut(Easing::Expo, t);
    }

    [[nodiscard]]
    inline double EaseInOutExpo(const double t) noexcept
    {
        return EaseInOut(Easing::Expo, t);
    }

    [[nodiscard]]
    inline double EaseInCirc(const double t) noexcept
    {
        return Easing::Circ(t);
    }

    [[nodiscard]]
    inline double EaseOutCirc(const double t) noexcept
    {
        return EaseOut(Easing::Circ, t);
    }

    [[nodiscard]]
    inline double EaseInOutCirc(const double t) noexcept
    {
        return EaseInOut(Easing::Circ, t);
    }

    [[nodiscard]]
    constexpr double EaseInBack(const double t) noexcept
    {
        return Easing::Back(t);
    }

    [[nodiscard]]
    constexpr double EaseOutBack(const double t) noexcept
    {
        return EaseOut(Easing::Back, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutBack(const double t) noexcept
    {
        return EaseInOut(Easing::Back, t);
    }

    [[nodiscard]]
    inline double EaseInElastic(const double t) noexcept
    {
        return Easing::Elastic(t);
    }

    [[nodiscard]]
    inline double EaseOutElastic(const double t) noexcept
    {
        return EaseOut(Easing::Elastic, t);
    }

    [[nodiscard]]
    inline double EaseInOutElastic(const double t) noexcept
    {
        return EaseInOut(Easing::Elastic, t);
    }

    [[nodiscard]]
    constexpr double EaseInBounce(const double t) noexcept
    {
        return Easing::Bounce(t);
    }

    [[nodiscard]]
    constexpr double EaseOutBounce(const double t) noexcept
    {
        return EaseOut(Easing::Bounce, t);
    }

    [[nodiscard]]
    constexpr double EaseInOutBounce(const double t) noexcept
    {
        return EaseInOut(Easing::Bounce, t);
    }
}
