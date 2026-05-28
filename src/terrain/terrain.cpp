#include "terrain.hpp"
#include <glm/glm.hpp>
#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

TerrainGenerator::TerrainGenerator(const TerrainParams& params)
    : m_params(params), m_noise(params.seed)
{
    m_heightmap = std::make_unique<Heightmap>(params.gridWidth, params.gridHeight);
    m_mesh      = std::make_unique<Mesh>();
}

void TerrainGenerator::generate() {
    buildHeightmap();
    buildMesh();
}

void TerrainGenerator::regenerate(const TerrainParams& params) {
    m_params    = params;
    m_noise     = NoiseGenerator(params.seed);
    m_heightmap = std::make_unique<Heightmap>(params.gridWidth, params.gridHeight);
    generate();
}

// ---------------------------------------------------------------------------
// Profil de cote (sigmoide)
// normalizedX : position en X dans [0, 1] sur la grille
// Retourne un facteur dans [-1, 1] : negatif = mer, positif = terre
// ---------------------------------------------------------------------------

float TerrainGenerator::coastProfile(float normalizedX) const {
    // Decalage par shoreOffset pour controler la position de la ligne de cote
    float x = (normalizedX - 0.5f - m_params.shoreOffset) / m_params.coastBlend;
    // Sigmoide douce (tanh)
    return std::tanh(x * 2.5f);
}

// ---------------------------------------------------------------------------
// Profil de falaise : accentue les hauteurs intermediaires
// Cree des falaises abruptes autour du niveau de la mer
// ---------------------------------------------------------------------------

float TerrainGenerator::cliffProfile(float rawHeight) const {
    // Courbe en S centree sur 0 : aplatit les sommets et les fonds
    // mais rend la transition tres abrupte
    float h = rawHeight;
    // Expansion des hautes terres
    if (h > 0.0f)
        h = std::pow(h, 0.7f);  // hauts plateaux plus plats
    else
        h = -std::pow(-h, 0.6f); // fond marin plus uniforme

    return h;
}

// ---------------------------------------------------------------------------
// Construction de la heightmap
// ---------------------------------------------------------------------------

void TerrainGenerator::buildHeightmap() {
    const int   W = m_params.gridWidth;
    const int   H = m_params.gridHeight;
    const float f = m_params.noiseFreq;

    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float nx = (float)x * f;
            float nz = (float)z * f;

            // 1. Bruit fractal avec domain warping pour des formes organiques
            float raw = m_noise.warpedFractal2D(
                nx, nz,
                m_params.octaves,
                m_params.warpStrength
            );
            // raw est dans [-1, 1]

            // 2. Profil de cote : masque en X pour creer la separation mer/terre
            float coast = coastProfile((float)x / (float)(W - 1));

            // Melange : la zone de transition melange le bruit et le gradient
            // Dans la mer (coast < 0) : le bruit est attenue
            // Sur la terre (coast > 0) : le bruit prend le dessus
            float blended = raw * 0.5f + coast * 0.5f;

            // 3. Profil de falaise pour accentuer la coupure terre/mer
            float height = cliffProfile(blended);

            // 4. Mise a l'echelle en metres
            m_heightmap->set(x, z, height * m_params.heightScale);
        }
    }

    // Upload vers la texture GPU pour les shaders et les futures compute shaders d'erosion
    m_heightmap->uploadToGPU();
}

// ---------------------------------------------------------------------------
// Construction du mesh OpenGL
// Grille reguliere de triangles, chaque sommet porte :
//   position (x, height, z), normal (vec3), uv (vec2)
// ---------------------------------------------------------------------------

void TerrainGenerator::buildMesh() {
    const int   W     = m_params.gridWidth;
    const int   H     = m_params.gridHeight;
    const float scale = m_params.worldScale / (float)(W - 1);

    // ---- Sommets ----
    // Chaque sommet : position (3) + normale (3) + uv (2) = 8 floats
    std::vector<float> vertices;
    vertices.reserve(W * H * 8);

    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float px = (float)x * scale - m_params.worldScale * 0.5f;
            float py = m_heightmap->get(x, z);
            float pz = (float)z * scale - m_params.worldScale * 0.5f;

            glm::vec3 n = m_heightmap->getNormal(x, z);

            float u = (float)x / (float)(W - 1);
            float v = (float)z / (float)(H - 1);

            vertices.insert(vertices.end(), {
                px, py, pz,      // position
                n.x, n.y, n.z,   // normale
                u, v             // uv
            });
        }
    }

    // ---- Indices (triangles) ----
    std::vector<unsigned int> indices;
    indices.reserve((W - 1) * (H - 1) * 6);

    for (int z = 0; z < H - 1; ++z) {
        for (int x = 0; x < W - 1; ++x) {
            unsigned int tl = z * W + x;
            unsigned int tr = tl + 1;
            unsigned int bl = tl + W;
            unsigned int br = bl + 1;

            // Triangle 1 : tl - bl - tr
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);

            // Triangle 2 : tr - bl - br
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    m_mesh->upload(vertices, indices);
}