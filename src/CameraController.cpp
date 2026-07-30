#include "CameraController.hpp"

#include "Game.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace
{
    const glm::vec3 DEFAULT_CAMERA_OFFSET{0.0f, 5.0f, 10.0f};

    constexpr float CAMERA_FOLLOW_SPEED{3.0f};
} // namespace

CameraController::CameraController(float screenWidth, float screenHeight)
    : camera(glm::vec3(0.0f, 5.0f, 10.0f)), lastMouseX(screenWidth / 2.0f),
      lastMouseY(screenHeight / 2.0f)
{
}

void CameraController::updateFollow(const Game& game, float deltaTime)
{
    const Player& player = game.getPlayer();
    const glm::vec2 playerPosition = player.getPosition();
    const glm::vec2 playerDirection = player.getCameraDirection();

    lookTarget = glm::vec3(playerPosition.x, 0.0f, playerPosition.y);

    glm::vec3 cameraOffset = DEFAULT_CAMERA_OFFSET;

    if (glm::length(playerDirection) > 0.01f)
    {
        cameraOffset = glm::vec3(-playerDirection.x * 10.0f, 5.0f, -playerDirection.y * 10.0f);
    }

    const glm::vec3 targetPosition = lookTarget + cameraOffset;

    const float followFactor = 1.0f - std::exp(-CAMERA_FOLLOW_SPEED * deltaTime);

    camera.Position = glm::mix(camera.Position, targetPosition, followFactor);
}

void CameraController::processMouseMovement(double xPosition, double yPosition)
{
    const float x = static_cast<float>(xPosition);
    const float y = static_cast<float>(yPosition);

    if (firstMouseMovement)
    {
        lastMouseX = x;
        lastMouseY = y;
        firstMouseMovement = false;
    }

    const float xOffset = x - lastMouseX;
    const float yOffset = lastMouseY - y;

    lastMouseX = x;
    lastMouseY = y;

    camera.ProcessMouseMovement(xOffset, yOffset);
}

void CameraController::processMouseScroll(double yOffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yOffset));
}

glm::mat4 CameraController::getViewMatrix() const
{
    return glm::lookAt(camera.Position, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

float CameraController::getZoom() const
{
    return camera.Zoom;
}