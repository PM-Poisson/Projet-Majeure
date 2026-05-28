#pragma once
#include <glm/glm.hpp>

// Camera orbitale + mode FPS
// - Mode orbite  : tourne autour d'un point cible avec la souris (clic droit)
// - Mode FPS     : deplacement ZQSD + souris libre (touche F pour basculer)
// - Zoom         : molette souris

class Camera {
public:
    // Mode actif
    enum class Mode { Orbit, FPS };

    Camera();

    // Matrices de vue et de projection
    glm::mat4 viewMatrix()       const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    // Position monde de la camera (pour l'eclairage)
    glm::vec3 position() const { return m_position; }

    // --- Inputs ---

    // Mouvement souris (delta en pixels)
    void onMouseMove(float dx, float dy, bool rightButtonHeld);

    // Molette : zoom en mode orbite, vitesse en mode FPS
    void onMouseScroll(float yOffset);

    // Clavier (appele chaque frame avec le deltaTime)
    void onKeyboard(bool fwd, bool back, bool left, bool right,
                    bool up, bool down, float deltaTime);

    // Bascule entre les deux modes
    void toggleMode();
    Mode mode() const { return m_mode; }

    // Parametres
    float fov         = 60.0f;
    float nearPlane   = 0.1f;
    float farPlane    = 2000.0f;
    float orbitSpeed  = 0.4f;
    float panSpeed    = 0.3f;
    float fpsSpeed    = 30.0f;
    float sensitivity = 0.15f;

private:
    Mode      m_mode   = Mode::Orbit;

    // --- Mode orbite ---
    glm::vec3 m_target   = glm::vec3(0.0f);
    float     m_orbitYaw = 30.0f;    // angle horizontal (degres)
    float     m_orbitPitch = 35.0f;  // angle vertical   (degres)
    float     m_orbitRadius = 150.0f;

    // --- Mode FPS ---
    glm::vec3 m_position = glm::vec3(0.0f, 60.0f, 120.0f);
    float     m_yaw      = -90.0f;
    float     m_pitch    = -20.0f;

    // Position calculee depuis les parametres orbitaux
    glm::vec3 orbitPosition() const;

    // Vecteurs FPS
    glm::vec3 fpsForward() const;
    glm::vec3 fpsRight()   const;
};