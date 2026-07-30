#pragma once

#include "GameEvent.hpp"

#include <filesystem>
#include <memory>

class AudioManager
{
public:
    explicit AudioManager(const std::filesystem::path& audioDirectory = "./assets/audio");
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) = delete;
    AudioManager& operator=(AudioManager&&) = delete;

    void play(GameEvent event);
    void toggleMuted();

    bool isMuted() const;
    bool isAvailable() const;

private:
    class Impl;
    std::unique_ptr<Impl> implementation;
};
