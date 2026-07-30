#include "../external/GLAD/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../external/shader_s.h"
#include "CameraController.hpp"
#include "FrameTimer.hpp"
#include "Game.hpp"
#include "GameRenderer.hpp"
#include "GlfwInput.hpp"

// GLFW callbacks and input handling.
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
    // Shared cube mesh used for tiles and characters.
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f, // Bottom-left
         0.5f, -0.5f,  0.5f, // Bottom-right
         0.5f,  0.5f,  0.5f, // Top-right
        -0.5f,  0.5f,  0.5f, // Top-left

        // Back face
        -0.5f, -0.5f, -0.5f, // Bottom-left
         0.5f, -0.5f, -0.5f, // Bottom-right
         0.5f,  0.5f, -0.5f, // Top-right
        -0.5f,  0.5f, -0.5f  // Top-left
    };

    unsigned int indices[] = {
        // Front face
        0, 1, 2,
        2, 3, 0,
        // Back face
        4, 5, 6,
        6, 7, 4,
        // Left face
        4, 7, 3,
        3, 0, 4,
        // Right face
        1, 5, 6,
        6, 2, 1,
        // Bottom face
        0, 1, 5,
        5, 4, 0,
        // Top face
        3, 2, 6,
        6, 7, 3
    };


    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW.\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PacBoy", NULL, NULL);

    if(window == nullptr)
    {
        std::cerr << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    CameraController cameraController(SCR_WIDTH, SCR_HEIGHT);
    glfwSetWindowUserPointer(window, &cameraController);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD.\n";
        glfwTerminate();
        return -1;
    }


    Game game({"./assets/levels/classic_inspired.txt"});
    if (!game.isInitialized())
    {
        glfwTerminate();
        return -1;
    }

    GameRenderer gameRenderer(SCR_WIDTH, SCR_HEIGHT);
    if (!gameRenderer.isValid())
    {
        glfwTerminate();
        return -1;
    }

    
    Shader ourShader("./shaders/shader.vs", "./shaders/shader.fs");
    if (!ourShader.isValid())
    {
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Upload cube geometry and configure the position attribute.
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    FrameTimer frameTimer;

    // Main loop
    while(!glfwWindowShouldClose(window))
    {   
        const float currentFrame =
            static_cast<float>(glfwGetTime());
        const float deltaTime =
            frameTimer.update(currentFrame);
        
        processInput(window);
        game.processPlayerInput(readGameInput(window), currentFrame);
        cameraController.updateFollow(game, deltaTime);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(cameraController.getZoom()), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = cameraController.getViewMatrix();
        int viewLoc = glGetUniformLocation(ourShader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        game.update(currentFrame, deltaTime);
        gameRenderer.render(game, ourShader, VAO);

        glfwSwapBuffers(window);
        glfwPollEvents();

        game.nextLevel(currentFrame);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(window, true);
    }
}

void mouse_callback(
    GLFWwindow* window,
    double xpos,
    double ypos
)
{
    auto* controller =
        static_cast<CameraController*>(
            glfwGetWindowUserPointer(window)
        );

    if (controller != nullptr)
    {
        controller->processMouseMovement(
            xpos,
            ypos
        );
    }
}

void scroll_callback(
    GLFWwindow* window,
    double,
    double yoffset
)
{
    auto* controller =
        static_cast<CameraController*>(
            glfwGetWindowUserPointer(window)
        );

    if (controller != nullptr)
    {
        controller->processMouseScroll(yoffset);
    }
}
