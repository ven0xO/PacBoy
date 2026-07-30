#pragma once

#include <array>

struct GameInput
{
    bool up{false};
    bool down{false};
    bool left{false};
    bool right{false};
    bool enter{false};
    bool backspace{false};
    bool pause{false};

    std::array<bool, 26> letters{};
};
