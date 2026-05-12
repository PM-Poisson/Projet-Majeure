#include "bruit.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>

// ---------------------------------------------------------------------------
// Construction / initialisation
// ---------------------------------------------------------------------------

NoiseGenerator::NoiseGenerator(uint32_t seed) {
    buildPermTable(seed);
}

void NoiseGenerator::buildPermTable(uint32_t seed) {
    // Remplit [0..255] puis melange avec la seed
    std::vector<int> base(256);
    std::iota(base.begin(), base.end(), 0);
    std::mt19937 rng(seed);
    std::shuffle(base.begin(), base.end(), rng);

    // Double la table pour eviter les modulos dans le code chaud
    for (int i = 0; i < 256; ++i)
        perm[i] = perm[i + 256] = base[i];
}

// ---------------------------------------------------------------------------
// Gradient 2D (12 directions regulieres)
// ---------------------------------------------------------------------------

float NoiseGenerator::grad2D(int hash, float x, float y) const {
    // 8 gradients sur un cercle unitaire
    static constexpr float G[8][2] = {
        { 1.0f,  0.0f}, {-1.0f,  0.0f},
        { 0.0f,  1.0f}, { 0.0f, -1.0f},
        { 0.707f, 0.707f}, {-0.707f,  0.707f},
        { 0.707f,-0.707f}, {-0.707f, -0.707f}
    };
    int h = hash & 7;
    return G[h][0] * x + G[h][1] * y;
}

// ---------------------------------------------------------------------------
// Simplex 2D
// Adapte de l'implementation de Stefan Gustavson (domaine public)
// ---------------------------------------------------------------------------

float NoiseGenerator::simplex2D(float x, float y) const {
    // Constantes de transformation skew/unskew
    static constexpr float F2 = 0.366025403f;  // (sqrt(3)-1)/2
    static constexpr float G2 = 0.211324865f;  // (3-sqrt(3))/6

    // Trouver le triangle simplex qui contient (x,y)
    float s  = (x + y) * F2;
    int   i  = (int)std::floor(x + s);
    int   j  = (int)std::floor(y + s);

    float t  = (float)(i + j) * G2;
    float X0 = (float)i - t;
    float Y0 = (float)j - t;
    float x0 = x - X0;
    float y0 = y - Y0;

    // Quel coin du triangle simplex sommes-nous dans ?
    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else          { i1 = 0; j1 = 1; }

    float x1 = x0 - (float)i1 + G2;
    float y1 = y0 - (float)j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    // Indices dans la table de permutation
    int ii = i & 255;
    int jj = j & 255;
    int g0 = perm[ii +       perm[jj      ]];
    int g1 = perm[ii + i1 +  perm[jj + j1 ]];
    int g2 = perm[ii + 1  +  perm[jj + 1  ]];

    // Contributions de chaque coin
    auto corner = [&](float cx, float cy, int g) -> float {
        float t2 = 0.5f - cx*cx - cy*cy;
        if (t2 < 0.0f) return 0.0f;
        t2 *= t2;
        return t2 * t2 * grad2D(g, cx, cy);
    };

    float n = corner(x0, y0, g0)
            + corner(x1, y1, g1)
            + corner(x2, y2, g2);

    // Normalise dans [-1, 1]  (facteur empirique pour Simplex 2D)
    return 70.0f * n;
}

// ---------------------------------------------------------------------------
// fBm : bruit fractal par accumulation d'octaves
// ---------------------------------------------------------------------------

float NoiseGenerator::fractal2D(float x, float y,
                                 int   octaves,
                                 float lacunarity,
                                 float gain) const {
    float value     = 0.0f;
    float amplitude = 0.5f;
    float frequency = 1.0f;
    float maxValue  = 0.0f;   // pour normaliser en fin de compte

    for (int i = 0; i < octaves; ++i) {
        value    += amplitude * simplex2D(x * frequency, y * frequency);
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return value / maxValue;   // retourne dans [-1, 1]
}

// ---------------------------------------------------------------------------
// Domain warping : tordu le domaine d'entree avec du bruit
// Cela produit des formes tres organiques (rivieres, falaises...)
// ---------------------------------------------------------------------------

float NoiseGenerator::warpedFractal2D(float x, float y,
                                       int   octaves,
                                       float warpScale) const {
    // Premiere passe : calcule un vecteur de deformation
    float warpX = fractal2D(x + 0.0f, y + 0.0f, octaves) * warpScale;
    float warpY = fractal2D(x + 5.2f, y + 1.3f, octaves) * warpScale;

    // Deuxieme passe : evalue le bruit dans l'espace tordu
    return fractal2D(x + warpX, y + warpY, octaves);
}