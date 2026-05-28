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
    float heightScale  = 30.0f;
    float seaLevel     = 0.0f;
    float islandRadius = 0.500f;
    float shoreBlend   = 0.30f;  // valeur médiane : modulée x0.8→x5 selon l'angle
    float noiseFreq    = 0.0066f;
    int   octaves      = 7;
    float warpStrength = 0.900f;
    uint32_t seed      = 1000;

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

    // Accesseurs pour le TreeSystem
    const std::vector<float>& heights()   const { return m_heights; }
    int                       gridN()     const { return params.gridN; }
    float                     worldSize() const { return params.worldSize; }
    float                     seaLevel()  const { return params.seaLevel; }

    IslandParams params;

private:
    Shader             m_shader;
    Mesh               m_mesh;
    NoiseGenerator     m_noise;
    std::vector<float> m_heights;

    void generateHeights();   // bruit + masque continu (sans cliffProfile)
    void applyErosion();
    void buildMesh();
};