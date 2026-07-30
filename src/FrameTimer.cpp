#include "FrameTimer.hpp"

#include <algorithm>

float FrameTimer::update(float currentFrame)
{
    const float deltaTime = std::min(
        currentFrame - lastFrame,
        MAX_DELTA_TIME
    );

    lastFrame = currentFrame;
    return deltaTime;
}
