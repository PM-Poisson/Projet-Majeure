#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdint>

#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"
#include "bruit.hpp"
#include "diamond_square.hpp"

// ===========================================================================
// TerrainMethod — choix de la méthode de génération du relief
//
//   SimplexNoise   : bruit Simplex fractal avec domain warping
//                    Produit un terrain organique et varié
//   DiamondSquare  : subdivision récursive (algorithme Diamond-Square)
//                    Produit un terrain fractal aux caractéristiques
//                    contrôlées par l'exposant de Hurst H
// ===========================================================================
enum class TerrainMethod {
    SimplexNoise,
    DiamondSquare
};

// ===========================================================================
// IslandParams — tous les paramètres de génération de l'île
// ===========================================================================
struct IslandParams {
    // --- Grille ---
    int   gridN        = 300;     // résolution NxN de la heightmap CPU
    float worldSize    = 160.0f;  // taille de l'île en unités monde

    // --- Hauteur et forme globale ---
    float heightScale  = 28.0f;   // amplitude maximale du terrain (unités monde)
    float seaLevel     = 0.0f;    // niveau de référence de la mer (Y=0 par défaut)
    float islandRadius = 0.44f;   // rayon moyen du plateau [0..1] en coords normalisées
    float shoreBlend   = 0.18f;   // largeur de la transition plateau->mer
                                  //   petit => falaise abrupte
                                  //   grand => plage en pente douce
    float cliffiness   = 0.22f;   // amplitude de l'asymétrie angulaire
                                  //   0   => île parfaitement ronde
                                  //   0.3 => une face en falaise, l'autre en plage

    // --- Paramètres Simplex ---
    float    noiseFreq    = 0.003f; // fréquence spatiale du bruit de base
    int      octaves      = 7;      // nombre d'octaves fBm
    float    warpStrength = 1.2f;   // intensité du domain warping (torsion du bruit)

    // --- Paramètres Diamond-Square ---
    int   dsSteps     = 8;     // niveaux de subdivision => grille (2^n+1)^2
                               //   8 => 257x257, 9 => 513x513
    float dsRoughness = 0.55f; // exposant de Hurst H in (0,1)
                               //   0.2 = très rugueux (montagnes chaotiques)
                               //   0.8 = lisse (collines douces)

    uint32_t seed = 42;
};

// ===========================================================================
// IslandScene — gère la génération et le rendu de l'île procédurale
//
// Pipeline de génération (commun aux deux méthodes) :
//   1. Générer une heightmap brute [-1,1] (Simplex ou Diamond-Square)
//   2. Appliquer le masque d'île radial asymétrique (localRadius)
//   3. Calculer le profil de rivage (shoreProfile) selon la distance au bord
//   4. Mélanger bruit et forme
//   5. Appliquer cliffProfile (accentue les falaises)
//   6. Mettre à l'échelle par heightScale
// ===========================================================================
class IslandScene {
public:
    IslandScene();

    // Initialise le shader et génère le mesh
    void init(const IslandParams& p = {}, TerrainMethod method = TerrainMethod::SimplexNoise);

    // Dessine l'île avec les matrices caméra et paramètres de lumière fournis
    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              const glm::vec3& lightDir,
              const glm::vec3& cameraPos);

    // Régénère le mesh avec de nouveaux paramètres (appelé depuis l'UI)
    void regenerate(const IslandParams& p, TerrainMethod method);

    IslandParams  params;
    TerrainMethod method = TerrainMethod::SimplexNoise;

private:
    Shader         m_shader;
    Mesh           m_mesh;
    NoiseGenerator m_noise;       // bruit pour la surface du terrain
    NoiseGenerator m_noiseShape;  // bruit séparé pour la forme de l'île (seed décalée)

    void buildMesh();

    // Applique le masque d'île + profils sur une heightmap brute
    std::vector<float> applyIslandMask(const std::vector<float>& rawHeights, int N) const;

    // Génère la heightmap brute selon la méthode choisie
    std::vector<float> generateSimplex(int N) const;
    std::vector<float> generateDiamondSquare() const;

    // Rayon effectif de l'île au point (nx, nz) :
    // modulé par un bruit BF pour créer l'asymétrie falaise/plage
    float localRadius(float nx, float nz) const;

    // Profil de hauteur en fonction de la distance signée au bord de l'île.
    // beachFactor (0..1) : 0 = falaise directe, 1 = plage large
    float shoreProfile(float distToEdge, float beachFactor) const;

    // Accentue les falaises : aplatit les sommets et les fonds,
    // rend la transition très abrupte autour du niveau de la mer
    float cliffProfile(float rawHeight) const;
};