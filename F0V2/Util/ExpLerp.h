#pragma once

#include <algorithm>
#include <cmath>

namespace Util
{
    /// @brief Computes a frame-rate-independent alpha for interpolation.
    float ExpAlpha(float alpha, float deltaTime);

    float FastExpAlpha(float alpha, float deltaTime);

    /// @remark 60 FPS 基準
    template <class T>
    T ExpLerp(
        const T& current,
        const T& target,
        float alpha,
        float deltaTime)
    {
        return std::lerp(
            current,
            target,
            ExpAlpha(alpha, deltaTime));
    }

    template <class T>
    T FastExpLerp(
        const T& current,
        const T& target,
        float alpha,
        float deltaTime)
    {
        return std::lerp(
            current,
            target,
            FastExpAlpha(alpha, deltaTime));
    }
}
