#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>

// ---------------------------------------------------------------------------
// DiamondSquare — Generateur de heightmap par subdivision recursive
//
// Reference : Galin et al., "A Review of Digital Terrain Modeling",
//             EUROGRAPHICS 2019 STAR, Section 3.1.1.
//             Algorithme original : Fournier, Fussell, Carpenter [FFC82b]
//
// Principe (deux etapes alternees) :
//
//   Etape Diamond :
//     Pour chaque carre de cote s, on calcule le point central comme
//     la moyenne des 4 coins du carre, auquel on ajoute un deplacement
//     aleatoire d'ecart-type sigma.
//
//   Etape Square :
//     Pour chaque losange (diamond) forme par 4 points adjacents, on
//     calcule le point central comme la moyenne des 4 extremites du
//     losange + deplacement aleatoire.
//
//   A chaque niveau de subdivision k :
//     sigma_k = sigma_0 * 2^(-k * H)
//   ou H in (0,1) est l'exposant de Hurst (dimension fractale D = 3 - H).
//     H proche de 1 => terrain lisse (plaines)
//     H proche de 0 => terrain tres rugueux (montagnes)
//
// La grille doit avoir une taille (2^n + 1) x (2^n + 1).
// On choisit n = steps, ce qui donne une resolution de 2^steps + 1.
// ---------------------------------------------------------------------------

struct DiamondSquareParams {
    int      steps       = 8;     // iterations => grille (2^steps+1)^2
                                  //   8 => 257x257, 9 => 513x513
    float    roughness   = 0.55f; // exposant de Hurst H in (0,1)
                                  //   0.3 = tres rugueux, 0.7 = lisse
    float    initAmpl    = 1.0f;  // amplitude initiale (sigma_0)
    uint32_t seed        = 42;
};

class DiamondSquare {
public:
    // Genere une heightmap de taille (2^params.steps + 1)^2
    // Les valeurs retournees sont dans [-1, 1] (non mises a l'echelle)
    static std::vector<float> generate(const DiamondSquareParams& p);

    // Taille de la grille pour un nombre d'etapes donne
    static int gridSize(int steps) { return (1 << steps) + 1; }

private:
    static float randDisplace(float sigma, uint32_t& state);
    static float get(const std::vector<float>& g, int n, int x, int z);
    static void  set(std::vector<float>& g, int n, int x, int z, float v);
};