#include "../external/GLAD/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../external/shader_s.h"
#include "AudioManager.hpp"
#include "CameraController.hpp"
#include "FrameTimer.hpp"
#include "Game.hpp"
#include "GameRenderer.hpp"
#include "GlfwInput.hpp"

void processInput(GLFWwindow& window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

namespace
{
    class GlfwGuard
    {
    public:
        GlfwGuard() = default;

        GlfwGuard(const GlfwGuard&) = delete;
        GlfwGuard& operator=(const GlfwGuard&) = delete;

        ~GlfwGuard()
        {
            glfwTerminate();
        }
    };
} // namespace

int main()
{
    struct CubeVertex
    {
        float x;
        float y;
        float z;
        float normalX;
        float normalY;
        float normalZ;
    };

    // Shared cube mesh. Every face has its own vertices so it can have a flat normal.
    const CubeVertex vertices[] = {
        // Position                    // Normal
        // Front
        {-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
        {0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
        {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
        {-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f},
        // Back
        {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f},
        {-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f},
        {-0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f},
        {0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f},
        // Left
        {-0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f},
        {-0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f},
        {-0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f},
        {-0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f},
        // Right
        {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f},
        {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
        {0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f},
        {0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f},
        // Bottom
        {-0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f},
        {0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f},
        {0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f},
        {-0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f},
        // Top
        {-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f},
        {0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f},
        {0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f},
    };

    const unsigned int indices[] = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Left
        12, 13, 14, 14, 15, 12, // Right
        16, 17, 18, 18, 19, 16, // Bottom
        20, 21, 22, 22, 23, 20, // Top
    };

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }

    GlfwGuard glfwGuard;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PacBoy", nullptr, nullptr);

    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window.\n";
        return -1;
    }

    glfwMakeContextCurrent(window);
    CameraController cameraController(SCR_WIDTH, SCR_HEIGHT);
    glfwSetWindowUserPointer(window, &cameraController);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD.\n";
        return -1;
    }

    Game game({"./assets/levels/classic_inspired.txt"});
    if (!game.isInitialized())
    {
        return -1;
    }

    GameRenderer gameRenderer(SCR_WIDTH, SCR_HEIGHT);
    if (!gameRenderer.isValid())
    {
        return -1;
    }

    Shader ourShader("./shaders/shader.vs", "./shaders/shader.fs");
    if (!ourShader.isValid())
    {
        return -1;
    }

    const int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");

    const int viewLoc = glGetUniformLocation(ourShader.ID, "view");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.005f, 0.008f, 0.025f, 1.0f);

    // Upload cube geometry and configure position and normal attributes.
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), nullptr);
    glEnableVertexAttribArray(0);

    // OpenGL represents a byte offset into the currently bound VBO as a pointer.
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    const void* normalOffset = reinterpret_cast<void*>(3 * sizeof(float));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CubeVertex), normalOffset);
    glEnableVertexAttribArray(1);

    FrameTimer frameTimer;
    AudioManager audioManager;
    bool previousMuteKey{false};

    while (!glfwWindowShouldClose(window))
    {
        const float currentFrame = static_cast<float>(glfwGetTime());
        const float deltaTime = frameTimer.update(currentFrame);

        processInput(*window);
        const GameInput input = readGameInput(*window);

        if (input.mute && !previousMuteKey && game.getPhase() != GamePhase::NewScore)
        {
            audioManager.toggleMuted();
            glfwSetWindowTitle(window, audioManager.isMuted() ? "PacBoy - MUTED" : "PacBoy");
        }

        previousMuteKey = input.mute;
        game.processPlayerInput(input, currentFrame);
        cameraController.updateFollow(game, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(cameraController.getZoom()),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = cameraController.getViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        game.update(currentFrame, deltaTime);
        gameRenderer.render(game, ourShader, VAO, currentFrame);

        glfwSwapBuffers(window);
        glfwPollEvents();

        game.nextLevel(currentFrame);

        for (const GameEvent event : game.takeEvents())
        {
            audioManager.play(event);
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    return 0;
}

void processInput(GLFWwindow& window)
{
    if (glfwGetKey(&window, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(&window, true);
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));

    if (controller != nullptr)
    {
        controller->processMouseMovement(xpos, ypos);
    }
}

void scroll_callback(GLFWwindow* window, double, double yoffset)
{
    auto* controller = static_cast<CameraController*>(glfwGetWindowUserPointer(window));

    if (controller != nullptr)
    {
        controller->processMouseScroll(yoffset);
    }
}
