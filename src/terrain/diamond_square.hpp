#pragma once

#include <vector>
#include <cstdint>

// ===========================================================================
// DiamondSquare — génération de terrain par subdivision récursive
//
// Référence : Galin et al., "A Review of Digital Terrain Modeling",
//             EUROGRAPHICS 2019 STAR, Section 3.1.1.
//             Algorithme original : Fournier, Fussell, Carpenter [FFC82b]
//
// Principe :
//   On part d'une grille (2^n+1)² avec les 4 coins initialisés aléatoirement.
//   À chaque niveau de subdivision, deux passes alternées :
//
//   Étape Diamond :
//     Le centre de chaque carré = moyenne des 4 coins + perturbation aléatoire
//
//   Étape Square :
//     Le milieu de chaque arête = moyenne des voisins en croix + perturbation
//
//   À chaque niveau k, l'amplitude de la perturbation diminue :
//     sigma_k = sigma_0 * 2^(-k * H)
//   où H est l'exposant de Hurst (roughness).
//
//   H proche de 0 => terrain très rugueux (dimension fractale D proche de 3)
//   H proche de 1 => terrain lisse (D proche de 2)
// ===========================================================================

struct DiamondSquareParams {
    int      steps     = 8;     // niveaux de subdivision => grille (2^steps+1)²
    float    roughness = 0.55f; // exposant de Hurst H ∈ (0,1)
    float    initAmpl  = 1.0f;  // amplitude initiale des coins
    uint32_t seed      = 42;
};

class DiamondSquare {
public:
    // Génère une heightmap de taille (2^steps+1)², valeurs dans [-1, 1]
    static std::vector<float> generate(const DiamondSquareParams& p);

    // Taille de la grille pour un nombre de subdivisions donné
    static int gridSize(int steps) { return (1 << steps) + 1; }

private:
    // Perturbation aléatoire dans [-sigma, +sigma] via un LCG rapide
    static float randDisplace(float sigma, uint32_t& state);

    // Accès avec wrapping périodique (pour les bords)
    static float get(const std::vector<float>& g, int n, int x, int z);
    static void  set(std::vector<float>& g, int n, int x, int z, float v);
};