#pragma once

#include "GameInput.hpp"

struct GLFWwindow;

// Borrows an already validated GLFW window for the duration of the call.
GameInput readGameInput(GLFWwindow& window);
