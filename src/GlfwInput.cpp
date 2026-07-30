#include "GlfwInput.hpp"

#include <GLFW/glfw3.h>
#include <cstddef>

GameInput readGameInput(GLFWwindow& window)
{
    GameInput input;

    input.up = glfwGetKey(&window, GLFW_KEY_UP) == GLFW_PRESS;
    input.down = glfwGetKey(&window, GLFW_KEY_DOWN) == GLFW_PRESS;
    input.left = glfwGetKey(&window, GLFW_KEY_LEFT) == GLFW_PRESS;
    input.right = glfwGetKey(&window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    input.enter = glfwGetKey(&window, GLFW_KEY_ENTER) == GLFW_PRESS;
    input.backspace = glfwGetKey(&window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    input.pause = glfwGetKey(&window, GLFW_KEY_P) == GLFW_PRESS;

    for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; ++key)
    {
        const std::size_t index = static_cast<std::size_t>(key - GLFW_KEY_A);

        input.letters[index] =
            glfwGetKey(&window, key) == GLFW_PRESS;
    }

    return input;
}
