#pragma once
#include <memory>

namespace TY
{
    namespace Audio
    {
        void SetEnabled(bool enabled);

        void SetSoundVolume(float volume);

        void SetMusicVolume(float volume);

        void StopAllSounds();

        void StopMusic(float fadeOutDuration = 0.5f);
    }

    struct AudioLoopRange
    {
        float beginSec{};
        float endSec{};
    };

    class SoundAudio
    {
    public:
        SoundAudio() = default;

        SoundAudio(const std::string& path);

        void setLoopEnabled(bool enabled);

        void playUnique(float volume = 1.0f) const;

        bool isPlayingUnique() const;

        void setUniqueVolume(float volume);

        void stopUnique(float fadeOutDuration = 0.5f) const;

        void playOneShot(float volume = 1.0f) const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };

    class MusicAudio
    {
    public:
        MusicAudio() = default;

        MusicAudio(const std::string& path);

        void play() const;

        void stop(float fadeOutDuration = 0.5f) const;

        void setLoop(AudioLoopRange loop);

        void setLoopAndTransition(AudioLoopRange loop);

        [[nodiscard]]
        float posSec() const;

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
