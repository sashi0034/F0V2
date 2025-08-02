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
    float StandardDeltaTime();

    [[nodiscard]]
    float InGameDeltaTime();

    [[nodiscard]]
    float RealDeltaTime();

    [[nodiscard]]
    float StandardElapsedTime();

    [[nodiscard]]
    float InGameElapsedTime();

    [[nodiscard]]
    float RealElapsedTime();

    [[nodiscard]]
    float GetDeltaTime(GameTime gameTime);

    void SetStandardTimeScale(float scale);

    void SetInGameTimeScale(float scale);

    void SetStandardTimeThreshold(float threshold);

    void SetInGameTimeThreshold(float threshold);

    [[nodiscard]]
    float GetStandardTimeScale();

    [[nodiscard]]
    float GetInGameTimeScale();

    [[nodiscard]]
    float GetTimeScale(GameTime gameTime);

    void ResetTimeScale();
}
