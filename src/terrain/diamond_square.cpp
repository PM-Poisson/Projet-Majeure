#include "diamond_square.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Generateur congruentiel lineaire (rapide, suffisant pour du bruit visuel)
// Retourne un float dans [-sigma, +sigma]
// ---------------------------------------------------------------------------
float DiamondSquare::randDisplace(float sigma, uint32_t& state) {
    // LCG classique
    state = state * 1664525u + 1013904223u;
    // Convertit en float dans [0, 1]
    float r = float(state) / float(0xFFFFFFFFu);
    // Centre sur 0 : [-sigma, +sigma]
    return (r * 2.0f - 1.0f) * sigma;
}

// Acces avec wrapping pour les bords (terrain periodique)
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
// generate
//
// Implémentation de l'algorithme Diamond-Square tel que décrit dans :
// Galin et al. 2019, Section 3.1.1, algorithme [FFC82b].
//
// La grille a une taille N = 2^steps + 1.
// Les 4 coins sont initialises aleatoirement.
// A chaque etape de subdivision :
//   1. Diamond step : point central de chaque carre
//   2. Square step  : point milieu de chaque arete
//   sigma diminue d'un facteur 2^(-H) a chaque etape.
// ---------------------------------------------------------------------------
std::vector<float> DiamondSquare::generate(const DiamondSquareParams& p) {
    if (p.steps < 1 || p.steps > 12)
        throw std::invalid_argument("steps doit etre entre 1 et 12");

    const int N    = gridSize(p.steps);  // 2^steps + 1
    const int Nm1  = N - 1;              // = 2^steps (taille utile du carre)

    std::vector<float> grid(N * N, 0.0f);

    uint32_t rng = p.seed;

    // --- Initialisation des 4 coins ---
    float s0 = p.initAmpl;
    set(grid, N, 0,   0,   randDisplace(s0, rng));
    set(grid, N, Nm1, 0,   randDisplace(s0, rng));
    set(grid, N, 0,   Nm1, randDisplace(s0, rng));
    set(grid, N, Nm1, Nm1, randDisplace(s0, rng));

    // --- Boucle de subdivision ---
    // A l'etape k, le pas courant est step = 2^(steps-k)
    // sigma_k = initAmpl * 2^(-k * H)  (decroit avec k)

    float sigma = p.initAmpl;

    for (int k = 0; k < p.steps; ++k) {
        int step  = Nm1 >> k;       // pas courant = 2^(steps-k)
        int half  = step >> 1;      // demi-pas

        // Mise a jour de sigma pour ce niveau
        // sigma_{k+1} = sigma_k * 2^(-H)
        sigma *= std::pow(2.0f, -p.roughness);

        // ---- Etape Diamond ----
        // Pour chaque carre de cote 'step', calculer le centre
        for (int z = 0; z < Nm1; z += step) {
            for (int x = 0; x < Nm1; x += step) {
                // 4 coins du carre courant
                float tl = get(grid, N, x,        z       );
                float tr = get(grid, N, x + step, z       );
                float bl = get(grid, N, x,        z + step);
                float br = get(grid, N, x + step, z + step);

                // Centre = moyenne des 4 coins + perturbation aleatoire
                float cx = x + half;
                float cz = z + half;
                set(grid, N, cx, cz,
                    (tl + tr + bl + br) * 0.25f + randDisplace(sigma, rng));
            }
        }

        // ---- Etape Square ----
        // Pour chaque losange (diamond), calculer le milieu de chaque arete
        for (int z = 0; z <= Nm1; z += half) {
            for (int x = (z + half) % step; x <= Nm1; x += step) {
                // Moyenne des 4 voisins en croix (certains peuvent etre hors grille)
                float sum   = 0.0f;
                int   count = 0;

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

    // --- Normalisation dans [-1, 1] ---
    float vmin = *std::min_element(grid.begin(), grid.end());
    float vmax = *std::max_element(grid.begin(), grid.end());
    float range = vmax - vmin;
    if (range > 1e-6f) {
        for (float& v : grid)
            v = (v - vmin) / range * 2.0f - 1.0f;
    }

    return grid;
}