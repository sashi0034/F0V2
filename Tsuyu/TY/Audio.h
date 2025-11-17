#pragma once
#include <memory>

namespace TY
{
    class SoundAudio
    {
    public:
        SoundAudio() = default;

        SoundAudio(const std::string& path);

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

    private:
        struct Impl;
        std::shared_ptr<Impl> p_impl;
    };
}
