#include "bruit.hpp"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <random>

// ─── Gradients 2D (12 directions) ───────────────────────────────────────────
static const float GRAD2[12][2] = {
    { 1, 1}, {-1, 1}, { 1,-1}, {-1,-1},
    { 1, 0}, {-1, 0}, { 1, 0}, {-1, 0},
    { 0, 1}, { 0,-1}, { 0, 1}, { 0,-1}
};

static constexpr float F2 = 0.3660254037844387f;  // (sqrt(3)-1)/2
static constexpr float G2 = 0.21132486540518713f; // (3-sqrt(3))/6

// ─── Constructeur ────────────────────────────────────────────────────────────
NoiseGenerator::NoiseGenerator(uint32_t seed) {
    initPermutations(seed);
}

void NoiseGenerator::initPermutations(uint32_t seed) {
    // Remplit [0..255] puis shuffle déterministe
    std::iota(perm, perm + 256, 0);
    std::mt19937 rng(seed);
    std::shuffle(perm, perm + 256, rng);

    // Double la table et précalcule mod12
    for (int i = 0; i < 256; ++i) {
        perm[256 + i]    = perm[i];
        permMod12[i]     = perm[i] % 12;
        permMod12[256+i] = permMod12[i];
    }
}

// ─── Produit scalaire gradient ────────────────────────────────────────────────
float NoiseGenerator::grad2D(int hash, float x, float y) const {
    int h = hash % 12;
    return GRAD2[h][0] * x + GRAD2[h][1] * y;
}

// ─── Simplex 2D ──────────────────────────────────────────────────────────────
float NoiseGenerator::simplex2D(float x, float y) const {
    // Skew vers l'espace simplex
    float s  = (x + y) * F2;
    int   i  = (int)std::floor(x + s);
    int   j  = (int)std::floor(y + s);
    float t  = (float)(i + j) * G2;

    // Coins du triangle en espace d'entrée
    float x0 = x - (i - t);
    float y0 = y - (j - t);

    // Quel triangle simplex ?
    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else          { i1 = 0; j1 = 1; }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    // Hachage des coins
    int ii = i & 255;
    int jj = j & 255;
    int gi0 = permMod12[ii      + perm[jj     ]];
    int gi1 = permMod12[ii + i1 + perm[jj + j1]];
    int gi2 = permMod12[ii + 1  + perm[jj + 1 ]];

    // Contribution de chaque coin
    auto contrib = [](float cx, float cy, int gi) -> float {
        float t = 0.5f - cx*cx - cy*cy;
        if (t < 0.0f) return 0.0f;
        t *= t;
        return t * t * (GRAD2[gi][0] * cx + GRAD2[gi][1] * cy);
    };

    float n0 = contrib(x0, y0, gi0);
    float n1 = contrib(x1, y1, gi1);
    float n2 = contrib(x2, y2, gi2);

    // Mise à l'échelle vers [-1, 1]
    return 70.0f * (n0 + n1 + n2);
}

// ─── Génération de la heightmap complète ─────────────────────────────────────
std::vector<float> NoiseGenerator::generateHeightmap(int   width,
                                                      int   height,
                                                      float scale,
                                                      int   octaves,
                                                      float lacunarity,
                                                      float persistence) const
{
    std::vector<float> hmap(width * height);

    float invW = 1.0f / (float)width;
    float invH = 1.0f / (float)height;

    float minV =  1e9f, maxV = -1e9f;

    // Première passe : calcul brut
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float nx = (float)x * invW * scale;
            float ny = (float)y * invH * scale;
            float v  = fbm(nx, ny, octaves, lacunarity, persistence);
            hmap[y * width + x] = v;
            if (v < minV) minV = v;
            if (v > maxV) maxV = v;
        }
    }

    // Deuxième passe : normalisation [0, 1]
    float range = maxV - minV;
    if (range < 1e-6f) range = 1.0f;
    for (float& v : hmap)
        v = (v - minV) / range;

    return hmap;
}