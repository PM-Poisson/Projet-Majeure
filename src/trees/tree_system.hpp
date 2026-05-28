#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

#include "../renderer/shader.hpp"
#include "../terrain/bruit.hpp"

// ---------------------------------------------------------------------------
// Paramètres de placement des arbres
// ---------------------------------------------------------------------------
struct TreePlacementParams {
    bool  enabled       = true;

    // Filtres biologiques
    float minHeight     = 1.5f;     // au-dessus du rivage (unités monde)
    float maxHeight     = 28.0f;    // sous la zone alpine
    float maxSlopeCos   = 0.75f;    // composante Y minimale de la normale
                                    // (1=plat, 0=vertical ; ici ≈49° max)

    // Échantillonnage
    float gridSpacing   = 1.8f;     // pas de la grille de candidats
    float jitterFactor  = 0.65f;    // perturbation aléatoire [0, 1]
    float minDistance   = 1.4f;     // distance minimale entre 2 arbres
    float density       = 0.75f;    // multiplicateur global [0, 1]
    float densityFreq      = 0.010f; // fréquence du bruit de clairières (basse = grandes zones)
    float clearingBias     = 0.15f;  // décalage du bruit vers le bas → plus de clairières
                                     // 0 = peu de clairières, 0.5 = moitié forêt/moitié clairière
    float clearingContrast = 1.5f;   // puissance du contraste → bordures forêt/clairière nettes
                                     // 1 = dégradé flou, 3+ = transition très abrupte

    // Variation visuelle
    float minScale      = 1.0f;
    float maxScale      = 2.2f;
};

struct TreeInstance {
    glm::vec3 position;
    float     scale;
    glm::vec3 foliageTint;
    float     rotation;
};

// ---------------------------------------------------------------------------
// TreeSystem : génère et rend des arbres procéduraux par instancing
// ---------------------------------------------------------------------------
class TreeSystem {
public:
    TreeSystem();
    ~TreeSystem();

    // Doit être appelé après création du contexte OpenGL
    void init();

    // Génère le placement et upload les instances dans le VBO instancié
    void generate(const std::vector<float>& heights, int N,
                  float worldSize, float seaLevel,
                  uint32_t seed,
                  const TreePlacementParams& p);

    void draw(const glm::mat4& view, const glm::mat4& proj,
              const glm::vec3& lightDir, const glm::vec3& cameraPos);

    int count() const { return (int)m_instances.size(); }

    TreePlacementParams params;

private:
    Shader m_shader;

    GLuint m_vao     = 0;
    GLuint m_vbo     = 0;   // mesh statique (pos, normal, color)
    GLuint m_ebo     = 0;
    GLuint m_instVbo = 0;   // attributs per-instance (pos, scale, tint, rot)
    int    m_indexCount = 0;

    std::vector<TreeInstance> m_instances;

    // Construit le mesh d'un sapin : tronc cylindrique + 3 cônes empilés
    void buildBaseMesh();

    // Upload les instances dans m_instVbo
    void uploadInstances();
};