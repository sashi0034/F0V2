#include "pch.h"
#include "ExpLerp.h"

#include <array>
#include <cassert>
#include <cmath>

namespace
{
    constexpr float BaseFps = 60.0f;
    constexpr float MaxDeltaTime = 0.1f;

    constexpr size_t AlphaResolution = 64;
    constexpr size_t DeltaTimeResolution = 64;

    using ExpLerpTable = std::array<std::array<float, DeltaTimeResolution + 1>, AlphaResolution + 1>;

    const ExpLerpTable& GetExpLerpTable()
    {
        static const ExpLerpTable table = []
        {
            ExpLerpTable result{};

            for (size_t a = 0; a <= AlphaResolution; ++a)
            {
                const float alpha =
                    static_cast<float>(a) / AlphaResolution;

                for (size_t d = 0; d <= DeltaTimeResolution; ++d)
                {
                    const float deltaTime =
                        MaxDeltaTime * static_cast<float>(d) / DeltaTimeResolution;

                    result[a][d] =
                        1.0f - std::pow(1.0f - alpha, deltaTime * BaseFps);
                }
            }

            return result;
        }();

        return table;
    }
}

namespace Util
{
    float ExpAlpha(float alpha, float deltaTime)
    {
        return 1.0f - std::pow(1.0f - alpha, deltaTime * BaseFps);
    }

    float FastExpAlpha(float alpha, float deltaTime)
    {
        assert(0.0f <= alpha && alpha <= 1.0f);
        assert(0.0f <= deltaTime);

        if (deltaTime > MaxDeltaTime)
        {
            // フレーム落ちなどでテーブルの範囲外になったときは、正確な計算にフォールバック
            return ExpAlpha(alpha, deltaTime);
        }

        alpha = std::clamp(alpha, 0.0f, 1.0f);
        deltaTime = std::clamp(deltaTime, 0.0f, MaxDeltaTime);

        const float alphaPosition =
            alpha * AlphaResolution;

        const float deltaTimePosition =
            deltaTime / MaxDeltaTime *
            DeltaTimeResolution;

        const size_t alphaIndex = std::min(
            static_cast<size_t>(alphaPosition),
            AlphaResolution - 1);

        const size_t deltaTimeIndex = std::min(
            static_cast<size_t>(deltaTimePosition),
            DeltaTimeResolution - 1);

        const float alphaFraction =
            alphaPosition - static_cast<float>(alphaIndex);

        const float deltaTimeFraction =
            deltaTimePosition - static_cast<float>(deltaTimeIndex);

        const auto& table = GetExpLerpTable();

        const float lower = std::lerp(
            table[alphaIndex][deltaTimeIndex],
            table[alphaIndex + 1][deltaTimeIndex],
            alphaFraction);

        const float upper = std::lerp(
            table[alphaIndex][deltaTimeIndex + 1],
            table[alphaIndex + 1][deltaTimeIndex + 1],
            alphaFraction);

        return std::lerp(
            lower,
            upper,
            deltaTimeFraction);
    }
}
