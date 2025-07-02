#pragma once
#include "GameTime.h"

namespace TY
{
    constexpr float Dt_120Hz = 1.0 / 120;
    constexpr float Dt_60Hz = 1.0 / 60;
    constexpr float Dt_30Hz = 1.0 / 30;
    constexpr float Dt_15Hz = 1.0 / 15;

    class GameStep
    {
    public:
        class Iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = float;
            using difference_type = std::ptrdiff_t;
            using pointer = const float*;
            using reference = const float&;

            explicit Iterator(int count, float dt);

            reference operator*() const;

            Iterator& operator++();

            bool operator!=(const Iterator& other) const;

        private:
            int m_count{};
            float m_dt{};
        };

        explicit GameStep(int step, float dt);

        GameStep& timeScaled(float timeScale);

        GameStep& timeScaled(GameTime gameTime);

        Iterator begin() const;

        Iterator end() const;

        /// @brief ステップ可能な状態であるか
        bool operator()() const;

    private:
        int m_step{};
        float m_dt{};
    };

    /// @brief 一定間隔で実行したい処理に用いるタイマー
    class GameStepTimer
    {
    public:
        GameStepTimer() = default;

        GameStepTimer(float fixedDeltaTime) : m_fixedDeltaTime(fixedDeltaTime) { return; }

        void Tick(float currentDeltaTime = InGameDeltaTime());

        GameStep::Iterator begin() const;

        GameStep::Iterator end() const;

        GameStep get() const;

    private:
        float m_fixedDeltaTime{};
        float m_fraction{};
        int m_step{};
    };

    [[nodiscard]]
    GameStep StandardStep_120Hz();

    [[nodiscard]]
    GameStep StandardStep_60Hz();

    [[nodiscard]]
    GameStep StandardStep_30Hz();

    [[nodiscard]]
    GameStep StandardStep_15Hz();
}
