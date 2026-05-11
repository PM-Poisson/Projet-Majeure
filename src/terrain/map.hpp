#pragma once
#include <vector>
//#include <glm/glm.hpp>

// Encapsule la heightmap et fournit des utilitaires
// (interpolation, normales, découpage LOD)
struct Heightmap {
    int   width  = 0;
    int   height = 0;
    float seaLevel = 0.35f;   // [0,1] — seuil eau/terre

    std::vector<float> data;  // [0,1] row-major

    // Accesseurs
    float at(int x, int y) const { return data[y * width + x]; }
    float& at(int x, int y)      { return data[y * width + x]; }

    // Hauteur interpolée (bilinéaire) pour des coords flottantes
    float sample(float x, float y) const;

    // Normale en un point (différences finies)
    //glm::vec3 normalAt(int x, int y, float verticalScale) const;

    // Vrai si le point est sous le niveau de la mer
    bool isSea(int x, int y) const { return at(x, y) < seaLevel; }
};