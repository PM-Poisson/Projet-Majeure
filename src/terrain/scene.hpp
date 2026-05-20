#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"
#include "bruit.hpp"

struct IslandParams {
    int   gridN        = 300;
    float worldSize    = 160.0f;
    float heightScale  = 35.0f;
    float seaLevel     = 0.0f;
    float islandRadius = 0.42f;  // rayon du plateau [0..1]
    float shoreBlend   = 0.5f;  // largeur falaise : petit=abrupt, grand=doux
    float noiseFreq    = 0.003f;
    int   octaves      = 7;
    float warpStrength = 1.2f;
    uint32_t seed      = 42;
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
    Shader         m_shader;
    Mesh           m_mesh;
    NoiseGenerator m_noise;

    void  buildMesh();
    float radialCoastProfile(float d) const;
    float cliffProfile(float rawHeight) const;
};