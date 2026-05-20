#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"

class Ocean {
public:
    Ocean();

    void init();

    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              float            time,
              const glm::vec3& lightDir,
              const glm::vec3& cameraPos);

    int   gridResolution = 120;
    float worldSize      = 200.0f;

    // Decalage vertical du plan de base de l'ocean.
    // Les vagues de Gerstner ajoutent ~4 unites vers le haut.
    // Avec -3.5, le niveau moyen visible est ~+0.5 unites,
    // ce qui laisse la plage (Y~0..2) visible au bord de l'eau.
    float oceanBaseY     = -3.5f;

private:
    Shader m_shader;
    Mesh   m_mesh;
    void   buildGrid();
};