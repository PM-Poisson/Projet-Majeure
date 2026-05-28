#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera() {
    // Position initiale : vue de dessus-cote, legerement en hauteur
    m_position = orbitPosition();
}

// ---------------------------------------------------------------------------
// Matrices
// ---------------------------------------------------------------------------

glm::vec3 Camera::orbitPosition() const {
    float yawRad   = glm::radians(m_orbitYaw);
    float pitchRad = glm::radians(m_orbitPitch);

    return m_target + glm::vec3(
        m_orbitRadius * std::cos(pitchRad) * std::sin(yawRad),
        m_orbitRadius * std::sin(pitchRad),
        m_orbitRadius * std::cos(pitchRad) * std::cos(yawRad)
    );
}

glm::vec3 Camera::fpsForward() const {
    float yawRad   = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);
    return glm::normalize(glm::vec3(
        std::cos(pitchRad) * std::cos(yawRad),
        std::sin(pitchRad),
        std::cos(pitchRad) * std::sin(yawRad)
    ));
}

glm::vec3 Camera::fpsRight() const {
    return glm::normalize(glm::cross(fpsForward(), glm::vec3(0, 1, 0)));
}

glm::mat4 Camera::viewMatrix() const {
    if (m_mode == Mode::Orbit) {
        glm::vec3 pos = orbitPosition();
        return glm::lookAt(pos, m_target, glm::vec3(0, 1, 0));
    } else {
        return glm::lookAt(m_position,
                           m_position + fpsForward(),
                           glm::vec3(0, 1, 0));
    }
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}





// ---------------------------------------------------------------------------
// Inputs souris
// ---------------------------------------------------------------------------

void Camera::onMouseMove(float dx, float dy, bool rightButtonHeld) {
    if (m_mode == Mode::Orbit) {
        if (!rightButtonHeld) return;
        m_orbitYaw   += dx * orbitSpeed;
        m_orbitPitch  = std::clamp(m_orbitPitch - dy * orbitSpeed, -89.0f, 89.0f);
    } else {
        m_yaw   += dx * sensitivity;
        m_pitch  = std::clamp(m_pitch - dy * sensitivity, -89.0f, 89.0f);
    }
}

void Camera::onMouseScroll(float yOffset) {
    if (m_mode == Mode::Orbit) {
        m_orbitRadius = std::clamp(m_orbitRadius - yOffset * 5.0f, 10.0f, 800.0f);
    } else {
        fpsSpeed = std::clamp(fpsSpeed + yOffset * 2.0f, 5.0f, 200.0f);
    }
}

// ---------------------------------------------------------------------------
// Inputs clavier (mode FPS uniquement)
// ---------------------------------------------------------------------------

void Camera::onKeyboard(bool fwd, bool back, bool left, bool right,
                        bool up, bool down, float deltaTime)
{
    if (m_mode != Mode::FPS) return;

    float v = fpsSpeed * deltaTime;
    if (fwd)   m_position += fpsForward() * v;
    if (back)  m_position -= fpsForward() * v;
    if (right) m_position += fpsRight()   * v;
    if (left)  m_position -= fpsRight()   * v;
    if (up)    m_position += glm::vec3(0, 1, 0) * v;
    if (down)  m_position -= glm::vec3(0, 1, 0) * v;
}

// ---------------------------------------------------------------------------
// Bascule de mode
// ---------------------------------------------------------------------------

void Camera::toggleMode() {
    if (m_mode == Mode::Orbit) {
        // Conserve la position courante en passant en FPS
        m_position = orbitPosition();
        m_yaw      = m_orbitYaw - 90.0f;
        m_pitch    = -m_orbitPitch;
        m_mode     = Mode::FPS;
    } else {
        m_mode = Mode::Orbit;
    }
}