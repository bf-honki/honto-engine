#pragma once

#include <string>

namespace honto
{
    class Audio
    {
    public:
        static bool PlayWav(const std::string& path, bool loop = false);
        static bool PlayAlias(const std::string& alias, bool loop = false);
        static bool PlayOnBus(const std::string& bus, const std::string& path, bool loop = false);
        static bool PlayMusic(const std::string& path, bool loop = true);
        static bool PlayEffect(const std::string& path);
        static void Stop();
        static void StopBus(const std::string& bus);
        static void SetMasterVolume(float volume);
        static float GetMasterVolume();
        static void SetBusVolume(const std::string& bus, float volume);
        static float GetBusVolume(const std::string& bus);
        static void PlayTone(int frequency, int durationMs);
    };
}
