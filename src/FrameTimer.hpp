#pragma once

class FrameTimer
{
public:
    float update(float currentFrame);

private:
    float lastFrame{0.0f};

    static constexpr float MAX_DELTA_TIME{0.05f};
};