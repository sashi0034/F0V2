#include "pch.h"
#include "Audio.h"

#include <soloud_wavstream.h>

#include "IComponent.h"
#include "Logger.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "System.h"
#include "detail/ComponentManager_singleton.h"

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "ole32.lib")

using namespace TY;

namespace
{
    struct AudioComponent;

    AudioComponent* s_audioComponent;

    struct MusicHandle
    {
        void* owner{};
        SoLoud::WavStream* wavStreamRef{};
        SoLoud::handle handle{};
    };

    struct AudioState
    {
        bool enabled{true};

        SoLoud::Soloud global; // Engine
        SoLoud::Bus sound;
        SoLoud::Bus music;

        MusicHandle currentMusic{};
    } s_audioState{};

    struct AudioComponent : IComponent
    {
        bool init() override
        {
            if (s_audioState.global.init() != SoLoud::SO_NO_ERROR)
            {
                LogError("AudioState: Failed to initialize SoLoud");
                return false;
            }

            s_audioState.global.play(s_audioState.sound);
            s_audioState.global.play(s_audioState.music);
            s_audioComponent = this;
            return true;
        }

        bool update() override
        {
            controlMusicLoop();

            return true;
        }

        ~AudioComponent()
        {
            s_audioState.global.deinit();
        }

    private:
        void controlMusicLoop()
        {
            if (s_audioState.currentMusic.owner == nullptr)
            {
                return;
            }

            float startSec = 1.0f; // TODO
            float endSec = 10.0f; // TODO
            if (s_audioState.global.getStreamPosition(s_audioState.currentMusic.handle) >= endSec)
            {
                s_audioState.global.stop(s_audioState.currentMusic.handle);

                const auto newHandle = s_audioState.music.play(
                    *s_audioState.currentMusic.wavStreamRef, 1.0f, 0.0f, true);

                s_audioState.currentMusic.handle = newHandle;

                if (const auto err = s_audioState.global.seek(newHandle, startSec);
                    err != SoLoud::SO_NO_ERROR)
                {
                    LogError("AudioState: Failed to seek music stream: {}", err);
                    return;
                }

                s_audioState.global.setPause(newHandle, false);
            }
        }
    };
}

struct SoundAudio::Impl
{
    SoLoud::Wav m_wav{};
    float m_lastPlayedTime{};

    bool Init(const std::string& path)
    {
        if (m_wav.load(path.data()) != SoLoud::SO_NO_ERROR)
        {
            LogError("SoundAudio: Failed to load sound from path: {}", path);
            return false;
        }

        return true;
    }

    void PlayOneShot(float volume)
    {
        if (not s_audioState.enabled)
        {
            return;
        }

        constexpr float minInterval = 0.1f; // seconds
        if (System::Time() - minInterval < m_lastPlayedTime)
        {
            return;
        }

        m_lastPlayedTime = System::Time();

        const auto handle = s_audioState.sound.play(m_wav);
        s_audioState.global.setVolume(handle, volume);
    }
};

struct MusicAudio::Impl
{
    SoLoud::WavStream m_wavStream{};
    SoLoud::handle m_handle{};

    bool Init(const std::string& path)
    {
        if (m_wavStream.load(path.data()) != SoLoud::SO_NO_ERROR)
        {
            LogError("MusicAudio: Failed to load music from path: {}", path);
            return false;
        }

        return true;
    }

    void Play()
    {
        if (not s_audioState.enabled)
        {
            return;
        }

        m_handle = s_audioState.music.play(m_wavStream);

        s_audioState.currentMusic = {this, &m_wavStream, m_handle};
    }
};

namespace TY
{
    void Audio::SetEnabled(bool enabled)
    {
        s_audioState.enabled = enabled;

        // TODO: Pause all sounds/music if disabled
    }

    SoundAudio::SoundAudio(const std::string& path)
        : p_impl(std::make_shared<Impl>())
    {
        if (not p_impl->Init(path))
        {
            p_impl.reset();
        }
    }

    void SoundAudio::playOneShot(float volume) const
    {
        if (p_impl)
        {
            p_impl->PlayOneShot(volume);
        }
    }

    MusicAudio::MusicAudio(const std::string& path)
        : p_impl(std::make_shared<Impl>())
    {
        if (not p_impl->Init(path))
        {
            p_impl.reset();
        }
    }

    void MusicAudio::play() const
    {
        if (p_impl)
        {
            p_impl->Play();
        }
    }

    namespace detail
    {
        void InitAudioComponent()
        {
            ComponentManager_singleton::Register<AudioComponent>("AudioComponent");
        }
    }
}
