#pragma once

#include <string>

namespace honto
{
    class Audio
    {
    public:
        static bool PlayWav(const std::string& path, bool loop = false);
        static bool PlayAlias(const std::string& alias, bool loop = false);
        static void Stop();
        static void PlayTone(int frequency, int durationMs);
    };
}
