#include "AudioManager.hpp"

#include <miniaudio.h>

#include <array>
#include <iostream>
#include <string>

namespace
{
    struct SoundDefinition
    {
        GameEvent event;
        const char* filename;
    };

    constexpr std::array<SoundDefinition, 8> SOUND_DEFINITIONS{{
        {GameEvent::MenuNavigate, "menu_navigate.wav"},
        {GameEvent::MenuSelect, "menu_select.wav"},
        {GameEvent::PelletCollected, "pellet.wav"},
        {GameEvent::EnergizerCollected, "energizer.wav"},
        {GameEvent::GhostEaten, "ghost_eaten.wav"},
        {GameEvent::PlayerDamaged, "damage.wav"},
        {GameEvent::LevelCompleted, "level_complete.wav"},
        {GameEvent::GameOver, "game_over.wav"},
    }};
} // namespace

class AudioManager::Impl
{
public:
    explicit Impl(const std::filesystem::path& audioDirectory)
    {
        if (ma_engine_init(nullptr, &engine) != MA_SUCCESS)
        {
            std::cerr << "Audio disabled: failed to initialize the audio device.\n";
            return;
        }

        engineInitialized = true;

        bool failedToLoad = false;
        for (std::size_t index = 0; index < SOUND_DEFINITIONS.size(); ++index)
        {
            const std::string path = (audioDirectory / SOUND_DEFINITIONS[index].filename).string();
            const ma_uint32 flags =
                MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;

            if (ma_sound_init_from_file(&engine, path.c_str(), flags, nullptr, nullptr,
                                        &sounds[index].sound) != MA_SUCCESS)
            {
                failedToLoad = true;
                continue;
            }

            sounds[index].initialized = true;
        }

        if (failedToLoad)
        {
            std::cerr << "Audio disabled: required sound files could not be loaded.\n";
            shutdown();
            return;
        }

        available = true;
    }

    ~Impl()
    {
        shutdown();
    }

    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;
    Impl(Impl&&) = delete;
    Impl& operator=(Impl&&) = delete;

    void play(GameEvent event)
    {
        if (!available || muted)
        {
            return;
        }

        for (std::size_t index = 0; index < SOUND_DEFINITIONS.size(); ++index)
        {
            if (SOUND_DEFINITIONS[index].event != event)
            {
                continue;
            }

            ma_sound_stop(&sounds[index].sound);
            ma_sound_seek_to_pcm_frame(&sounds[index].sound, 0);
            ma_sound_start(&sounds[index].sound);
            return;
        }
    }

    void toggleMuted()
    {
        muted = !muted;

        if (engineInitialized)
        {
            ma_engine_set_volume(&engine, muted ? 0.0F : 1.0F);
        }
    }

    bool isMuted() const
    {
        return muted;
    }

    bool isAvailable() const
    {
        return available;
    }

private:
    struct Sound
    {
        ma_sound sound{};
        bool initialized{false};
    };

    ma_engine engine{};
    std::array<Sound, SOUND_DEFINITIONS.size()> sounds{};
    bool engineInitialized{false};
    bool available{false};
    bool muted{false};

    void shutdown()
    {
        available = false;

        for (auto& sound : sounds)
        {
            if (sound.initialized)
            {
                ma_sound_uninit(&sound.sound);
                sound.initialized = false;
            }
        }

        if (engineInitialized)
        {
            ma_engine_uninit(&engine);
            engineInitialized = false;
        }
    }
};

AudioManager::AudioManager(const std::filesystem::path& audioDirectory)
    : implementation(std::make_unique<Impl>(audioDirectory))
{
}

AudioManager::~AudioManager() = default;

void AudioManager::play(GameEvent event)
{
    implementation->play(event);
}

void AudioManager::toggleMuted()
{
    implementation->toggleMuted();
}

bool AudioManager::isMuted() const
{
    return implementation->isMuted();
}

bool AudioManager::isAvailable() const
{
    return implementation->isAvailable();
}
