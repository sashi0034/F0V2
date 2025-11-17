#include "pch.h"
#include "GameTime.h"

#include "Math.h"
#include "detail/ComponentManager_singleton.h"
#include "TY/Addon.h"
#include "TY/IAddon.h"
#include "TY/System.h"

using namespace TY;

namespace
{
    struct TimeState
    {
        float deltaTime{};
        float elapsedTime{};
        float timeScale = 1.0;
        float timeThreshold = FLT_MAX;
    };

    std::array<TimeState, GameTimeCategories_3> s_timeStates{};

    constexpr float defaultTimeThreshold = 1.0 / 20; // 20 FPS

    struct GameTimeComponent : IComponent
    {
        bool init() override
        {
            s_timeStates[static_cast<int>(GameTime::Standard)].timeThreshold = defaultTimeThreshold;
            s_timeStates[static_cast<int>(GameTime::InGame)].timeThreshold = defaultTimeThreshold;

            return true;
        }

        bool update() override
        {
            for (auto& state : s_timeStates)
            {
                state.deltaTime = state.timeScale * Min(System::DeltaTime(), state.timeThreshold);
                state.elapsedTime += state.deltaTime;
            }

            return true;
        }
    };
}

namespace TY
{
    float StandardDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].deltaTime;
    }

    float InGameDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].deltaTime;
    }

    float RealDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Real)].deltaTime;
    }

    float StandardElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].elapsedTime;
    }

    float InGameElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].elapsedTime;
    }

    float RealElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Real)].elapsedTime;
    }

    float GetDeltaTime(GameTime gameTime)
    {
        return s_timeStates[static_cast<int>(gameTime)].deltaTime;
    }

    void SetStandardTimeScale(float scale)
    {
        s_timeStates[static_cast<int>(GameTime::Standard)].timeScale = scale;
    }

    void SetInGameTimeScale(float scale)
    {
        s_timeStates[static_cast<int>(GameTime::InGame)].timeScale = scale;
    }

    void SetStandardTimeThreshold(float threshold)
    {
        s_timeStates[static_cast<int>(GameTime::Standard)].timeThreshold = threshold;
    }

    void SetInGameTimeThreshold(float threshold)
    {
        s_timeStates[static_cast<int>(GameTime::InGame)].timeThreshold = threshold;
    }

    float GetStandardTimeScale()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].timeScale;
    }

    float GetInGameTimeScale()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].timeScale;
    }

    float GetTimeScale(GameTime gameTime)
    {
        return s_timeStates[static_cast<int>(gameTime)].timeScale;
    }

    void ResetTimeScale()
    {
        for (auto& state : s_timeStates)
        {
            state.timeScale = 1.0;
        }
    }

    namespace detail
    {
        void InitGameTimeComponent()
        {
            ComponentManager_singleton::Register<GameTimeComponent>("GameTimeAddon");
        }
    }
}
