#pragma once
#include "map.hpp"
#include "bruit.hpp"
#include "../renderer/mesh.hpp"
#include <memory>

// Parametres de generation du terrain cotier
struct TerrainParams {
    // Dimensions
    int   gridWidth     = 512;   // nombre de sommets en X
    int   gridHeight    = 512;   // nombre de sommets en Z
    float worldScale    = 200.0f; // taille du terrain en metres

    // Hauteurs
    float heightScale   = 40.0f; // amplitude max des falaises
    float seaLevel      = 0.0f;  // niveau de la mer (0 = origine)

    // Bruit
    float noiseFreq     = 0.003f; // frequence de base du bruit
    int   octaves       = 7;
    float lacunarity    = 2.1f;
    float gain          = 0.48f;
    float warpStrength  = 1.2f;   // intensite du domain warping

    // Profil cotier
    float coastBlend    = 0.35f;  // largeur de la zone de transition mer/terre
    float shoreOffset   = 0.1f;   // position de la ligne de cote ([-1,1])

    // Seed
    uint32_t seed = 42;
};

// Genere le terrain cotier (falaises + mer) :
//  1. Construit la Heightmap via NoiseGenerator (CPU)
//  2. Applique un profil de cote (gradient pour separer terre/mer)
//  3. Construit le Mesh OpenGL correspondant
class TerrainGenerator {
public:
    explicit TerrainGenerator(const TerrainParams& params = {});

    // Genere completement : heightmap + mesh
    void generate();

    // Regenere avec de nouveaux parametres
    void regenerate(const TerrainParams& params);

    // Accesseurs
    Heightmap&        heightmap()  { return *m_heightmap; }
    Mesh&             mesh()       { return *m_mesh;      }
    const TerrainParams& params()  const { return m_params; }

private:
    TerrainParams              m_params;
    std::unique_ptr<Heightmap> m_heightmap;
    std::unique_ptr<Mesh>      m_mesh;
    NoiseGenerator             m_noise;

    void buildHeightmap();
    void buildMesh();

    // Profil de cote : courbe sigmoide qui force la separation terre/mer
    float coastProfile(float normalizedX) const;

    // Remappage de la hauteur pour accentuer les falaises
    float cliffProfile(float rawHeight) const;
};