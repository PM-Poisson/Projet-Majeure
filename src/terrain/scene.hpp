#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"
#include "bruit.hpp"
#include "../erosion/erosion_system.hpp"

struct IslandParams {
    int   gridN        = 300;
    float worldSize    = 160.0f;
    float heightScale  = 35.0f;
    float seaLevel     = 0.0f;
    float islandRadius = 0.42f;
    float shoreBlend   = 0.12f;  // valeur médiane : modulée x0.25→x4 selon l'angle
    float noiseFreq    = 0.003f;
    int   octaves      = 7;
    float warpStrength = 1.2f;
    uint32_t seed      = 42;

    ErosionParams erosion;
};

class IslandScene {
public:
    IslandScene();

    void init      (const IslandParams& p = {});
    void draw      (const glm::mat4& view,
                    const glm::mat4& proj,
                    const glm::vec3& lightDir,
                    const glm::vec3& cameraPos);
    void regenerate(const IslandParams& p);

    IslandParams params;

private:
    Shader             m_shader;
    Mesh               m_mesh;
    NoiseGenerator     m_noise;
    std::vector<float> m_heights;

    float radialCoastProfile(float d, float localBlend) const;  // localBlend variable
    float cliffProfile(float rawHeight) const;

    void generateHeights();
    void applyErosion();
    void buildMesh();
};
