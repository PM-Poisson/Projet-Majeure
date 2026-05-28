#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>
#include <random>

#include "../renderer/shader.hpp"

// ---------------------------------------------------------------------------
// Paramètres de simulation des boids
// ---------------------------------------------------------------------------
struct FishParams {
    bool  enabled        = true;

    int   numSchools     = 3;      // nombre de bancs
    int   fishPerSchool  = 20;     // poissons par banc

    // Boids
    float cohesionRadius = 12.0f;  // rayon de cohésion (suit le centre du banc)
    float separationRadius= 2.5f;  // distance minimale entre 2 poissons
    float cohesionForce  = 0.8f;
    float alignmentForce = 0.6f;
    float separationForce= 1.5f;
    float explorationForce=0.4f;   // bruit aléatoire d'exploration

    // Mouvement
    float maxSpeed       = 4.0f;   // unités/seconde
    float minSpeed       = 1.0f;   // vitesse minimum (les poissons nagent toujours)

    // Confinement vertical
    float minDepth       = -14.0f; // hauteur Y minimale (pas trop profond)
    float maxDepth       = -0.5f;  // hauteur Y maximale (reste sous l'eau)
    float depthForce     = 3.0f;   // force de rappel vertical

    // Évitement terrain
    float terrainMargin  = 1.5f;   // distance au-dessus du terrain déclenchant l'évitement
    float terrainForce   = 8.0f;   // force d'évitement du terrain

    // Visuel
    float fishScale      = 0.35f;  // taille des poissons
};

// ---------------------------------------------------------------------------
// Un poisson
// ---------------------------------------------------------------------------
struct Fish {
    glm::vec3 pos;
    glm::vec3 vel;
    float     noiseOffset; // offset dans le bruit d'exploration (unique par poisson)
};

// ---------------------------------------------------------------------------
// Un banc
// ---------------------------------------------------------------------------
struct School {
    std::vector<Fish> fish;
    glm::vec3         color;      // teinte du banc
    glm::vec3         center;     // centre de masse (calculé chaque frame)
};

// ---------------------------------------------------------------------------
// FishSystem — simulation Boids + rendu instancié
// ---------------------------------------------------------------------------
class FishSystem {
public:
    FishSystem();
    ~FishSystem();

    // Appeler après init OpenGL
    void init();

    // Initialise les positions des bancs autour de l'île
    // heights : heightmap CPU de l'île (pour l'évitement terrain)
    void spawn(const std::vector<float>& heights, int gridN,
               float worldSize, float seaLevel,
               uint32_t seed,
               const FishParams& p);

    // Simulation + upload GPU chaque frame
    void update(float dt, const std::vector<float>& heights,
                int gridN, float worldSize);

    void draw(const glm::mat4& view, const glm::mat4& proj,
              const glm::vec3& lightDir, const glm::vec3& cameraPos);

    int  totalFish() const;
    FishParams params;

private:
    Shader m_shader;
    GLuint m_vao     = 0;
    GLuint m_vbo     = 0;    // mesh statique
    GLuint m_ebo     = 0;
    GLuint m_instVbo = 0;    // positions/orientations par instance (mis à jour chaque frame)
    int    m_indexCount = 0;

    std::vector<School> m_schools;
    std::mt19937        m_rng;
    float               m_time = 0.0f;

    // Heightmap gardée en référence pour l'évitement terrain
    const std::vector<float>* m_heights  = nullptr;
    int   m_gridN    = 0;
    float m_worldSize= 0.0f;
    float m_seaLevel = 0.0f;

    void buildFishMesh();
    void uploadInstances();

    // Échantillonne la hauteur terrain en position monde (x, z)
    float terrainHeightAt(float wx, float wz) const;

    // Calcule les forces Boids pour un poisson dans son banc
    glm::vec3 computeBoidForce(const Fish& fish, const School& school,
                               float noiseTime) const;

    // Force de confinement vertical + évitement terrain
    glm::vec3 computeEnvironmentForce(const Fish& fish) const;
};
