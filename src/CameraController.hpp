#pragma once

#include "../external/camera.h"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

class Game;

class CameraController
{
public:
    CameraController(float screenWidth, float screenHeight);

    void updateFollow(const Game& game, float deltaTime);
    void processMouseMovement(double xPosition, double yPosition);
    void processMouseScroll(double yOffset);
    glm::mat4 getViewMatrix() const;
    float getZoom() const;

private:
    Camera camera;
    glm::vec3 lookTarget{0.0f};

    float lastMouseX{0.0f};
    float lastMouseY{0.0f};
    bool firstMouseMovement{true};
};