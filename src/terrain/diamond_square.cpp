#include "diamond_square.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ---------------------------------------------------------------------------
// randDisplace — perturbation aléatoire dans [-sigma, +sigma]
//
// Utilise un générateur congruentiel linéaire (LCG), suffisant pour
// du bruit visuel et beaucoup plus rapide que std::mt19937.
// ---------------------------------------------------------------------------
float DiamondSquare::randDisplace(float sigma, uint32_t& state) {
    state = state * 1664525u + 1013904223u;                 // LCG classique
    float r = float(state) / float(0xFFFFFFFFu);            // [0, 1]
    return (r * 2.0f - 1.0f) * sigma;                      // [-sigma, +sigma]
}

// ---------------------------------------------------------------------------
// get / set — accès à la grille avec wrapping périodique
//
// Le wrapping évite les vérifications de bord dans les étapes Diamond/Square
// et rend le terrain potentiellement tileable.
// ---------------------------------------------------------------------------
float DiamondSquare::get(const std::vector<float>& g, int n, int x, int z) {
    x = ((x % n) + n) % n;
    z = ((z % n) + n) % n;
    return g[z * n + x];
}

void DiamondSquare::set(std::vector<float>& g, int n, int x, int z, float v) {
    if (x < 0 || x >= n || z < 0 || z >= n) return;
    g[z * n + x] = v;
}

// ---------------------------------------------------------------------------
// generate — algorithme Diamond-Square complet
//
// La grille a une taille N = 2^steps + 1.
// Les 4 coins sont initialisés avec une perturbation aléatoire.
//
// À chaque itération k (de 0 à steps-1) :
//   - step  = 2^(steps-k)  : taille du carré courant
//   - half  = step / 2     : demi-pas (distance au centre)
//   - sigma diminue selon sigma *= 2^(-H)
//
//   Étape Diamond : pour chaque carré de côté step,
//     grid[cx][cz] = moyenne(4 coins) + rand(sigma)
//
//   Étape Square  : pour chaque losange,
//     grid[mx][mz] = moyenne(voisins en croix) + rand(sigma)
//     (les voisins hors grille sont ignorés, pas de wrapping ici)
//
// La heightmap résultante est normalisée dans [-1, 1].
// ---------------------------------------------------------------------------
std::vector<float> DiamondSquare::generate(const DiamondSquareParams& p) {
    if (p.steps < 1 || p.steps > 12)
        throw std::invalid_argument("steps doit etre entre 1 et 12");

    const int N   = gridSize(p.steps);   // 2^steps + 1
    const int Nm1 = N - 1;               // = 2^steps

    std::vector<float> grid(N * N, 0.0f);
    uint32_t rng = p.seed;

    // Initialisation des 4 coins
    float s0 = p.initAmpl;
    set(grid, N, 0,   0,   randDisplace(s0, rng));
    set(grid, N, Nm1, 0,   randDisplace(s0, rng));
    set(grid, N, 0,   Nm1, randDisplace(s0, rng));
    set(grid, N, Nm1, Nm1, randDisplace(s0, rng));

    float sigma = p.initAmpl;

    for (int k = 0; k < p.steps; ++k) {
        int step = Nm1 >> k;       // taille du carré courant
        int half = step >> 1;      // demi-pas

        // sigma diminue à chaque niveau : sigma_{k+1} = sigma_k * 2^(-H)
        sigma *= std::pow(2.0f, -p.roughness);

        // --- Étape Diamond ---
        // Pour chaque carré de côté step, calcul du centre
        for (int z = 0; z < Nm1; z += step) {
            for (int x = 0; x < Nm1; x += step) {
                float tl = get(grid, N, x,        z       );
                float tr = get(grid, N, x + step, z       );
                float bl = get(grid, N, x,        z + step);
                float br = get(grid, N, x + step, z + step);

                // Centre = moyenne des 4 coins + perturbation
                set(grid, N, x + half, z + half,
                    (tl + tr + bl + br) * 0.25f + randDisplace(sigma, rng));
            }
        }

        // --- Étape Square ---
        // Pour chaque point milieu d'arête, moyenne des voisins en croix
        for (int z = 0; z <= Nm1; z += half) {
            for (int x = (z + half) % step; x <= Nm1; x += step) {
                float sum   = 0.0f;
                int   count = 0;

                // Collecte des voisins valides (bord : voisin absent ignoré)
                auto addNeighbor = [&](int nx, int nz) {
                    if (nx >= 0 && nx <= Nm1 && nz >= 0 && nz <= Nm1) {
                        sum += get(grid, N, nx, nz);
                        ++count;
                    }
                };

                addNeighbor(x - half, z      );
                addNeighbor(x + half, z      );
                addNeighbor(x,        z - half);
                addNeighbor(x,        z + half);

                if (count > 0)
                    set(grid, N, x, z,
                        sum / float(count) + randDisplace(sigma, rng));
            }
        }
    }

    // Normalisation finale dans [-1, 1]
    float vmin = *std::min_element(grid.begin(), grid.end());
    float vmax = *std::max_element(grid.begin(), grid.end());
    float range = vmax - vmin;
    if (range > 1e-6f)
        for (float& v : grid)
            v = (v - vmin) / range * 2.0f - 1.0f;

    return grid;
}