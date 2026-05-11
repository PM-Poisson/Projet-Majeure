#pragma once
#include <vector>
#include <cstdint>

// Simplex noise 2D/3D
class NoiseGenerator {
public:
    explicit NoiseGenerator(uint32_t seed = 42);

    // Simplex noise de base [-1, 1]
    float simplex2D(float x, float y) const;

    // Génère une heightmap complète (row-major, [0,1])
    // width/height : dimensions en samples
    // scale        : zoom du bruit (plus grand = plus zoomé)
    std::vector<float> generateHeightmap(int width, int height,
                                          float scale,
                                          int octaves,
                                          float lacunarity,
                                          float persistence) const;

private:
    void initPermutations(uint32_t seed);

    float grad2D(int hash, float x, float y) const;

    int  perm[512];       // table de permutations
    int  permMod12[512];  // perm % 12
};