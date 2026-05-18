#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"

// Ocean : plan XZ anime par des vagues (vertex shader)
// Convention : Y = hauteur, plan de base a Y = 0

class Ocean {
public:
    Ocean();

    // Charge le shader et construit la grille GPU
    void init();

    // Dessine l'ocean
    // view, proj : matrices camera  |  time : secondes depuis le debut
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              float            time);

    // Parametres de la grille
    int   gridResolution = 80;      // NxN sommets
    float worldSize      = 200.0f;  // taille en unites monde (meme echelle que le terrain)

private:
    Shader m_shader;
    Mesh   m_mesh;

    void buildGrid();
};