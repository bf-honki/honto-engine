#include "honto/Audio.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <mmsystem.h>

#include <string>
#include <thread>

#pragma comment(lib, "winmm.lib")

namespace
{
    std::wstring ToWide(const std::string& text)
    {
        if (text.empty())
        {
            return {};
        }

        const int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        if (required <= 0)
        {
            return {};
        }

        std::wstring result(static_cast<std::size_t>(required), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), required);
        if (!result.empty() && result.back() == L'\0')
        {
            result.pop_back();
        }

        return result;
    }
}

namespace honto
{
    bool Audio::PlayWav(const std::string& path, bool loop)
    {
        const std::wstring widePath = ToWide(path);
        if (widePath.empty())
        {
            return false;
        }

        DWORD flags = SND_FILENAME | SND_ASYNC;
        if (loop)
        {
            flags |= SND_LOOP;
        }

        return PlaySoundW(widePath.c_str(), nullptr, flags) == TRUE;
    }

    bool Audio::PlayAlias(const std::string& alias, bool loop)
    {
        const std::wstring wideAlias = ToWide(alias);
        if (wideAlias.empty())
        {
            return false;
        }

        DWORD flags = SND_ALIAS | SND_ASYNC;
        if (loop)
        {
            flags |= SND_LOOP;
        }

        return PlaySoundW(wideAlias.c_str(), nullptr, flags) == TRUE;
    }

    void Audio::Stop()
    {
        PlaySoundW(nullptr, nullptr, 0);
    }

    void Audio::PlayTone(int frequency, int durationMs)
    {
        const int safeFrequency = frequency <= 0 ? 440 : frequency;
        const int safeDuration = durationMs <= 0 ? 80 : durationMs;

        std::thread(
            [safeFrequency, safeDuration]()
            {
                Beep(safeFrequency, safeDuration);
            }
        ).detach();
    }
}
