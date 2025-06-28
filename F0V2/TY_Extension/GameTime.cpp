#include "pch.h"
#include "GameTime.h"

#include "TY/Addon.h"
#include "TY/IAddon.h"
#include "TY/System.h"

namespace
{
    struct TimeState
    {
        double deltaTime{};
        double elapsedTime{};
        double timeScale = 1.0;
    };

    std::array<TimeState, GameTimeCategories_3> s_timeStates{};

    struct GameTimeAddon : IAddon
    {
        bool init() override
        {
            return true;
        }

        bool update() override
        {
            for (auto& state : s_timeStates)
            {
                state.deltaTime = state.timeScale * System::DeltaTime();
                state.elapsedTime += state.deltaTime;
            }

            return true;
        }
    };
}

namespace TY
{
    double StandardDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].deltaTime;
    }

    double InGameDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].deltaTime;
    }

    double RealtimeDeltaTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Realtime)].deltaTime;
    }

    double StandardElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].elapsedTime;
    }

    double InGameElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].elapsedTime;
    }

    double RealtimeElapsedTime()
    {
        return s_timeStates[static_cast<int>(GameTime::Realtime)].elapsedTime;
    }

    double GetDeltaTime(GameTime gameTime)
    {
        return s_timeStates[static_cast<int>(gameTime)].deltaTime;
    }

    void SetStandardTimeScale(double scale)
    {
        s_timeStates[static_cast<int>(GameTime::Standard)].timeScale = scale;
    }

    void SetInGameTimeScale(double scale)
    {
        s_timeStates[static_cast<int>(GameTime::InGame)].timeScale = scale;
    }

    double GetStandardTimeScale()
    {
        return s_timeStates[static_cast<int>(GameTime::Standard)].timeScale;
    }

    double GetInGameTimeScale()
    {
        return s_timeStates[static_cast<int>(GameTime::InGame)].timeScale;
    }

    double GetTimeScale(GameTime gameTime)
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

    void InitGameTimeAddon()
    {
        Addon::Register<GameTimeAddon>("GameTimeAddon");
    }
}
