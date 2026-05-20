#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"
#include "bruit.hpp"
#include "diamond_square.hpp"

// ---------------------------------------------------------------------------
// Methode de generation du terrain
// ---------------------------------------------------------------------------
enum class TerrainMethod {
    SimplexNoise,    // Votre methode originale : bruit Simplex + domain warping
    DiamondSquare    // Subdivision recursive [Galin et al. 2019, Section 3.1.1]
};

// ---------------------------------------------------------------------------
// IslandParams — parametres communs aux deux methodes
// ---------------------------------------------------------------------------
struct IslandParams {
    // Grille
    int   gridN        = 300;     // resolution pour Simplex (NxN)
    float worldSize    = 160.0f;

    // Hauteur et forme
    float heightScale  = 28.0f;
    float seaLevel     = 0.0f;
    float islandRadius = 0.44f;
    float shoreBlend   = 0.18f;
    float cliffiness   = 0.22f;

    // --- Parametres Simplex ---
    float    noiseFreq    = 0.003f;
    int      octaves      = 7;
    float    warpStrength = 1.2f;

    // --- Parametres Diamond-Square ---
    // La grille sera de taille (2^dsSteps + 1)^2
    int   dsSteps     = 8;     // 8 => 257x257, 9 => 513x513
    float dsRoughness = 0.55f; // Exposant de Hurst H : 0=rugueux, 1=lisse
                                // Correspond a la "dimension fractale" du papier

    uint32_t seed = 42;
};

// ---------------------------------------------------------------------------
// IslandScene
// ---------------------------------------------------------------------------
class IslandScene {
public:
    IslandScene();

    void init(const IslandParams& p = {}, TerrainMethod method = TerrainMethod::SimplexNoise);
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& lightDir,
              const glm::vec3& cameraPos);
    void regenerate(const IslandParams& p, TerrainMethod method);

    IslandParams  params;
    TerrainMethod method = TerrainMethod::SimplexNoise;

private:
    Shader         m_shader;
    Mesh           m_mesh;
    NoiseGenerator m_noise;
    NoiseGenerator m_noiseShape;

    void buildMesh();

    // Pipeline commun apres generation de la heightmap brute :
    // applique le masque d'ile + cliffProfile + mise a l'echelle
    std::vector<float> applyIslandMask(const std::vector<float>& rawHeights,
                                       int N) const;

    // Generation selon la methode choisie
    std::vector<float> generateSimplex(int N) const;
    std::vector<float> generateDiamondSquare() const;

    // Profils (communs aux deux methodes)
    float localRadius   (float nx, float nz) const;
    float shoreProfile  (float distToEdge, float beachFactor) const;
    float cliffProfile  (float rawHeight) const;
};