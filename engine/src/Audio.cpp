#include "honto/Audio.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <mmsystem.h>

#include <algorithm>
#include <cwctype>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#pragma comment(lib, "winmm.lib")

namespace
{
    struct BusState
    {
        std::wstring alias;
        float volume = 1.0f;
        bool active = false;
    };

    std::mutex g_AudioMutex;
    float g_MasterVolume = 1.0f;
    std::unordered_map<std::string, BusState> g_Buses;

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

    std::wstring SanitizeAlias(const std::string& name)
    {
        std::wstring alias = L"honto_";
        for (char character : name)
        {
            if (std::isalnum(static_cast<unsigned char>(character)) != 0)
            {
                alias.push_back(static_cast<wchar_t>(::towlower(static_cast<wchar_t>(character))));
            }
            else
            {
                alias.push_back(L'_');
            }
        }

        if (alias == L"honto_")
        {
            alias += L"default";
        }

        return alias;
    }

    std::wstring QuoteForMci(std::wstring text)
    {
        for (wchar_t& character : text)
        {
            if (character == L'"')
            {
                character = L'\'';
            }
        }

        return L"\"" + text + L"\"";
    }

    bool SendMci(const std::wstring& command)
    {
        return mciSendStringW(command.c_str(), nullptr, 0, nullptr) == 0;
    }

    BusState& RequireBus(const std::string& bus)
    {
        auto found = g_Buses.find(bus);
        if (found == g_Buses.end())
        {
            BusState state;
            state.alias = SanitizeAlias(bus);
            found = g_Buses.emplace(bus, std::move(state)).first;
        }

        return found->second;
    }

    int EffectiveVolume(const std::string& bus)
    {
        const BusState& state = RequireBus(bus);
        const float volume = std::clamp(g_MasterVolume * state.volume, 0.0f, 1.0f);
        return static_cast<int>(volume * 1000.0f);
    }

    void ApplyBusVolume(const std::string& bus)
    {
        BusState& state = RequireBus(bus);
        if (!state.active)
        {
            return;
        }

        SendMci(L"setaudio " + state.alias + L" volume to " + std::to_wstring(EffectiveVolume(bus)));
    }

    bool OpenAudioAlias(const std::wstring& alias, const std::wstring& path)
    {
        const std::wstring quotedPath = QuoteForMci(path);
        return SendMci(L"open " + quotedPath + L" alias " + alias) ||
               SendMci(L"open " + quotedPath + L" type waveaudio alias " + alias) ||
               SendMci(L"open " + quotedPath + L" type mpegvideo alias " + alias);
    }
}

namespace honto
{
    bool Audio::PlayWav(const std::string& path, bool loop)
    {
        return PlayOnBus("effect", path, loop);
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

    bool Audio::PlayOnBus(const std::string& bus, const std::string& path, bool loop)
    {
        const std::wstring widePath = ToWide(path);
        if (widePath.empty())
        {
            return false;
        }

        std::lock_guard<std::mutex> lock(g_AudioMutex);
        BusState& state = RequireBus(bus);
        SendMci(L"close " + state.alias);

        if (!OpenAudioAlias(state.alias, widePath))
        {
            state.active = false;
            return false;
        }

        state.active = true;
        ApplyBusVolume(bus);
        return SendMci(loop ? (L"play " + state.alias + L" repeat") : (L"play " + state.alias));
    }

    bool Audio::PlayMusic(const std::string& path, bool loop)
    {
        return PlayOnBus("music", path, loop);
    }

    bool Audio::PlayEffect(const std::string& path)
    {
        return PlayOnBus("effect", path, false);
    }

    void Audio::Stop()
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        for (auto& [bus, state] : g_Buses)
        {
            (void)bus;
            SendMci(L"close " + state.alias);
            state.active = false;
        }

        PlaySoundW(nullptr, nullptr, 0);
    }

    void Audio::StopBus(const std::string& bus)
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        BusState& state = RequireBus(bus);
        SendMci(L"close " + state.alias);
        state.active = false;
    }

    void Audio::SetMasterVolume(float volume)
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        g_MasterVolume = std::clamp(volume, 0.0f, 1.0f);
        for (const auto& [bus, state] : g_Buses)
        {
            (void)state;
            ApplyBusVolume(bus);
        }
    }

    float Audio::GetMasterVolume()
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        return g_MasterVolume;
    }

    void Audio::SetBusVolume(const std::string& bus, float volume)
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        BusState& state = RequireBus(bus);
        state.volume = std::clamp(volume, 0.0f, 1.0f);
        ApplyBusVolume(bus);
    }

    float Audio::GetBusVolume(const std::string& bus)
    {
        std::lock_guard<std::mutex> lock(g_AudioMutex);
        return RequireBus(bus).volume;
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
