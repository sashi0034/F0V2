#include "pch.h"
#include "Audio.h"

#include "InlineComponent.h"
#include "Logger.h"
#include "soloud.h"
#include "soloud_wav.h"

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

using namespace TY;

namespace
{
    struct AudioState : IInlineComponent
    {
        SoLoud::Soloud soloud; // Engine
        SoLoud::Wav music;
        SoLoud::Wav sound;

        int TODO = 0;

        AudioState()
        {
            soloud.init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::XAUDIO2);

            LogInfo("Audio: Initialized");
        }
    };

    InlineComponent<AudioState> s_audioState{};
}

namespace TY
{
    Audio::Audio(std::string_view path)
    {
        s_audioState->TODO++;
    }
}
