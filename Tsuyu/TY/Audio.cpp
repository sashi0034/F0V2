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
    constexpr float defaultFadeOutDuration = 0.5f;

    constexpr float defaultFadeInDuration = 0.5f;

    struct AudioComponent;

    AudioComponent* s_audioComponent;

    struct MusicData
    {
        SoLoud::WavStream wavStream{};
        SoLoud::handle handle{};
        AudioLoopRange loopRange{};
    };

    struct AudioState
    {
        bool enabled{true};

        SoLoud::Soloud global; // Engine
        SoLoud::Bus sound;
        SoLoud::Bus music;

        std::shared_ptr<MusicData> currentMusic{};
    } s_audioState{};

    void playMusicFrom(MusicData& musicData, float fromSec, float fadeInDuration)
    {
        constexpr float toVolume = 1.0f;
        const float fromVolume = fadeInDuration > 0.0f ? 0.0f : toVolume;

        const auto newHandle = s_audioState.music.play(musicData.wavStream, fromVolume, 0.0f, true);

        musicData.handle = newHandle;

        s_audioState.global.seek(newHandle, fromSec);

        if (fadeInDuration > 0.0f)
        {
            s_audioState.global.fadeVolume(newHandle, toVolume, fadeInDuration);
        }

        s_audioState.global.setPause(newHandle, false);
    }

    void stopMusic(const MusicData& musicData, float fadeOutDuration)
    {
        if (fadeOutDuration > 0.0f)
        {
            s_audioState.global.fadeVolume(musicData.handle, 0.0f, fadeOutDuration);
            s_audioState.global.schedulePause(musicData.handle, fadeOutDuration);
        }
        else
        {
            s_audioState.global.stop(musicData.handle);
        }
    }

    void loopMusicIfNeeded(MusicData& musicData)
    {
        const auto [startSec, endSec] = musicData.loopRange;
        if (startSec == endSec)
        {
            return;
        }

        if (s_audioState.global.getStreamPosition(musicData.handle) <= endSec)
        {
            return;
        }

        // -----------------------------------------------
        // ループ実行

        if (s_audioState.global.seek(musicData.handle, startSec) == SoLoud::SO_NO_ERROR)
        {
            return;
        }

        // -----------------------------------------------
        // workaround: ループ出来ない場合は再作成

        s_audioState.global.stop(musicData.handle);

        playMusicFrom(musicData, startSec, 0.0f);
    }

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
            if (s_audioState.currentMusic == nullptr)
            {
                return;
            }

            loopMusicIfNeeded(*s_audioState.currentMusic);
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
    std::shared_ptr<MusicData> m_data{std::make_shared<MusicData>()};

    bool Init(const std::string& path)
    {
        if (m_data->wavStream.load(path.data()) != SoLoud::SO_NO_ERROR)
        {
            LogError("MusicAudio: Failed to load music from path: {}", path);
            return false;
        }

        return true;
    }

    ~Impl()
    {
        if (s_audioState.currentMusic == m_data)
        {
            s_audioState.currentMusic = nullptr;
        }
    }

    void Play()
    {
        if (not s_audioState.enabled)
        {
            return;
        }

        if (s_audioState.currentMusic)
        {
            stopMusic(*s_audioState.currentMusic, defaultFadeOutDuration);

            s_audioState.currentMusic = nullptr;
        }

        m_data->handle = s_audioState.music.play(m_data->wavStream);

        s_audioState.currentMusic = m_data;
    }

    void Stop(float fadeDuration)
    {
        if (s_audioState.currentMusic == m_data)
        {
            s_audioState.currentMusic = nullptr;
        }

        stopMusic(*m_data, fadeDuration);
    }

    void SetLoop(const AudioLoopRange& loopRange)
    {
        m_data->loopRange = loopRange;
    }

    void SetLoopAndTransition(const AudioLoopRange& loopRange)
    {
        m_data->loopRange = loopRange;

        if (s_audioState.currentMusic != m_data)
        {
            return;
        }

        s_audioState.global.fadeVolume(m_data->handle, 0.0f, defaultFadeOutDuration);
        s_audioState.global.schedulePause(m_data->handle, defaultFadeOutDuration);

        playMusicFrom(*m_data, m_data->loopRange.beginSec, defaultFadeInDuration);
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

    void MusicAudio::stop(float fadeOutDuration) const
    {
        if (p_impl)
        {
            p_impl->Stop(fadeOutDuration);
        }
    }

    void MusicAudio::setLoop(AudioLoopRange loop)
    {
        if (p_impl)
        {
            p_impl->SetLoop(loop);
        }
    }

    void MusicAudio::setLoopAndTransition(AudioLoopRange loop)
    {
        if (p_impl)
        {
            p_impl->SetLoopAndTransition(loop);
        }
    }

    float MusicAudio::posSec() const
    {
        return p_impl ? s_audioState.global.getStreamPosition(p_impl->m_data->handle) : 0.0f;
    }

    namespace detail
    {
        void InitAudioComponent()
        {
            ComponentManager_singleton::Register<AudioComponent>("AudioComponent");
        }
    }
}
