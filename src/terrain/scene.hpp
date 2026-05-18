#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"
#include "bruit.hpp"   // votre NoiseGenerator existant

// Ile generee procedurallement, centree en (0,0,0)
// Convention : Y = hauteur (meme repere que l'ocean)

struct IslandParams {
    int      gridN        = 256;     // resolution NxN
    float    worldSize    = 160.0f;  // taille en unites monde (< ocean 200)
    float    heightScale  = 40.0f;   // amplitude max des falaises
    float    islandRadius = 0.36f;   // rayon de l'ile (fraction de worldSize/2)
    float    shoreBlend   = 0.09f;   // largeur de la transition ile/mer
    float    noiseFreq    = 0.004f;
    int      octaves      = 6;
    float    warpStrength = 1.0f;
    uint32_t seed         = 42;
};

class IslandScene {
public:
    IslandScene();

    void init(const IslandParams& p = {});
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& lightDir,
              const glm::vec3& cameraPos);

    void regenerate(const IslandParams& p);

    IslandParams params;

private:
    Shader m_shader;
    Mesh   m_mesh;
    NoiseGenerator m_noise;   // votre classe bruit.hpp

    void buildMesh();
    float islandMask(float nx, float ny) const;
};