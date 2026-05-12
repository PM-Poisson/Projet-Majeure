#pragma once
#include <vector>
#include <cstdint>

// Generateur de bruit Simplex 2D/3D
// Utilise une table de permutation pour garantir la reproductibilite avec une seed
class NoiseGenerator {
public:
    explicit NoiseGenerator(uint32_t seed = 42);

    // Bruit Simplex 2D, retourne une valeur dans [-1, 1]
    float simplex2D(float x, float y) const;

    // Bruit fractal (fBm) : accumule plusieurs octaves de Simplex
    // octaves    : nombre de couches de detail
    // lacunarity : facteur de frequence entre octaves (typiquement 2.0)
    // gain       : facteur d'amplitude entre octaves (typiquement 0.5)
    float fractal2D(float x, float y,
                    int   octaves    = 6,
                    float lacunarity = 2.0f,
                    float gain       = 0.5f) const;

    // Bruit de domaine tordu (domain-warped fBm) pour des formes plus organiques
    float warpedFractal2D(float x, float y,
                          int   octaves    = 6,
                          float warpScale  = 0.8f) const;

private:
    int  perm[512];          // table de permutation doublée

    float grad2D(int hash, float x, float y) const;
    void  buildPermTable(uint32_t seed);
};