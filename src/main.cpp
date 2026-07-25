#include "../external/GLAD/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../external/shader_s.h"
#include "../external/camera.h"
#include "Game.hpp"

// GLFW callbacks and input handling.
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Camera
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f)); 
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// Frame timing.
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void checkFileExists(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cout << "File not found: " << path << std::endl;
    } else {
        std::cout << "File exists: " << path << std::endl;
    }
}

const glm::vec3 CAMERA_OFFSET = glm::vec3(0.0f, 5.0f, 10.0f);


int main()
{

    checkFileExists("./shaders/shader.vs");
    checkFileExists("./shaders/shader.fs");

    
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
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PacBoy", NULL, NULL);

    if(window == nullptr)
    {
        std::cout << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    Game game("./assets/levels/classic_inspired.txt", SCR_WIDTH, SCR_HEIGHT);

    
    Shader ourShader("./shaders/shader.vs", "./shaders/shader.fs");
    glEnable(GL_DEPTH_TEST);

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


    // Main loop
    while(!glfwWindowShouldClose(window))
    {   
        float currentFrame = glfwGetTime() - game.getTimerOffset();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        game.processPlayerInput(window);
        // Smoothly follow behind the player based on the current movement direction.
        glm::vec3 playerPos = glm::vec3(game.getPlayerPtr()->getPosition().x, 0.0f, game.getPlayerPtr()->getPosition().y);
        glm::vec2 playerDir = game.getPlayerPtr()->getCameraDirection();
        glm::vec3 cameraOffset = CAMERA_OFFSET;

        if (glm::length(playerDir) > 0.01f)
        {
            cameraOffset = glm::vec3(-playerDir.x, 0.0f, -playerDir.y) * glm::vec3(10.0f, 1.0f, 10.0f);
            cameraOffset.y = 5.0f;
        }
        glm::vec3 targetCameraPos = playerPos + cameraOffset;
        camera.Position = glm::mix(camera.Position, targetCameraPos, 0.05f);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ourShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        int projectionLoc = glGetUniformLocation(ourShader.ID, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = glm::lookAt(camera.Position, playerPos, glm::vec3(0.0f, 1.0f, 0.0f));
        int viewLoc = glGetUniformLocation(ourShader.ID, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        game.update(currentFrame);
        game.render(ourShader, VAO);

        glfwSwapBuffers(window);
        glfwPollEvents();

        game.nextLevel(lastFrame, currentFrame);
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

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Mouse y-coordinates are inverted.

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
