#pragma once

namespace TY
{
    enum class GameTime
    {
        /// @brief ゲーム内で用いる標準の時間
        Standard,

        /// @brief インゲーム用の時間
        InGame,

        /// @brief システムと同じ実時間
        Real,
    };

    constexpr int GameTimeCategories_3 = 3;

    [[nodiscard]]
    double StandardDeltaTime();

    [[nodiscard]]
    double InGameDeltaTime();

    [[nodiscard]]
    double RealDeltaTime();

    [[nodiscard]]
    double StandardElapsedTime();

    [[nodiscard]]
    double InGameElapsedTime();

    [[nodiscard]]
    double RealElapsedTime();

    [[nodiscard]]
    double GetDeltaTime(GameTime gameTime);

    void SetStandardTimeScale(double scale);

    void SetInGameTimeScale(double scale);

    [[nodiscard]]
    double GetStandardTimeScale();

    [[nodiscard]]
    double GetInGameTimeScale();

    [[nodiscard]]
    double GetTimeScale(GameTime gameTime);

    void ResetTimeScale();
}
