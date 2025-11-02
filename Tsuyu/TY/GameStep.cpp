#include "pch.h"
#include "GameStep.h"

#include "detail/ComponentManager_singleton.h"

using namespace TY;

namespace
{
    enum class HertzKind
    {
        T_120Hz,
        T_60Hz,
        T_30Hz,
        T_15Hz,

        Max,
    };

    constexpr int hertzCount = static_cast<int>(HertzKind::Max);

    std::array<GameStepTimer, hertzCount> s_standardSteps{};

    struct GameStepComponent : IComponent
    {
        bool init() override
        {
            s_standardSteps[static_cast<int>(HertzKind::T_120Hz)] = GameStepTimer(Dt_120Hz);
            s_standardSteps[static_cast<int>(HertzKind::T_60Hz)] = GameStepTimer(Dt_60Hz);
            s_standardSteps[static_cast<int>(HertzKind::T_30Hz)] = GameStepTimer(Dt_30Hz);
            s_standardSteps[static_cast<int>(HertzKind::T_15Hz)] = GameStepTimer(Dt_15Hz);

            return true;
        }

        bool update() override
        {
            for (int i = 0; i < s_standardSteps.size(); ++i)
            {
                const float dt = StandardDeltaTime();
                s_standardSteps[i].Tick(dt);
            }

            return true;
        }
    };
}

namespace TY
{
    GameStep::Iterator::Iterator(int count, float dt) :
        m_count(count),
        m_dt(dt)
    {
    }

    GameStep::Iterator::reference GameStep::Iterator::operator*() const
    {
        return m_dt;
    }

    GameStep::Iterator& GameStep::Iterator::operator++()
    {
        ++m_count;
        return *this;
    }

    bool GameStep::Iterator::operator!=(const Iterator& other) const
    {
        return m_count != other.m_count;
    }

    GameStep::GameStep(int step, float dt) :
        m_step(step),
        m_dt(dt)
    {
    }

    GameStep& GameStep::timeScaled(float timeScale)
    {
        m_dt *= timeScale;
        return *this;
    }

    GameStep& GameStep::timeScaled(GameTime gameTime)
    {
        return timeScaled(GetTimeScale(gameTime));
    }

    GameStep::Iterator GameStep::begin() const
    {
        return Iterator(0, m_dt);
    }

    GameStep::Iterator GameStep::end() const
    {
        return Iterator(m_step, m_dt);
    }

    bool GameStep::operator()() const
    {
        return m_step > 0;
    }

    void GameStepTimer::Tick(float currentDeltaTime)
    {
        m_fraction += currentDeltaTime;

        m_step = 0;
        while (m_fraction > m_fixedDeltaTime)
        {
            m_step++;
            m_fraction -= m_fixedDeltaTime;
        }

        // return *this;
    }

    GameStep::Iterator GameStepTimer::begin() const
    {
        return GameStep::Iterator(0, m_fixedDeltaTime);
    }

    GameStep::Iterator GameStepTimer::end() const
    {
        return GameStep::Iterator(m_step, m_fixedDeltaTime);
    }

    GameStep GameStepTimer::get() const
    {
        return GameStep(m_step, m_fixedDeltaTime);
    }

    GameStep StandardStep_120Hz()
    {
        return s_standardSteps[static_cast<int>(HertzKind::T_120Hz)].get();
    }

    GameStep StandardStep_60Hz()
    {
        return s_standardSteps[static_cast<int>(HertzKind::T_60Hz)].get();
    }

    GameStep StandardStep_30Hz()
    {
        return s_standardSteps[static_cast<int>(HertzKind::T_30Hz)].get();
    }

    GameStep StandardStep_15Hz()
    {
        return s_standardSteps[static_cast<int>(HertzKind::T_15Hz)].get();
    }

    namespace detail
    {
        void InitGameStepComponent()
        {
            ComponentManager_singleton::Register<GameStepComponent>("GameStepComponent");
        }
    }
}
