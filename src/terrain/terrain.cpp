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

float TerrainGenerator::coastProfile(float normalizedX) const {
    float x = (normalizedX - 0.5f - m_params.shoreOffset) / m_params.coastBlend;
    return std::tanh(x * 2.5f);
}

float TerrainGenerator::cliffProfile(float rawHeight) const {
    float h = rawHeight;
    if (h > 0.0f)
        h = std::pow(h, 0.7f);
    else
        h = -std::pow(-h, 0.6f);
    return h;
}

void TerrainGenerator::buildHeightmap() {
    const int   W = m_params.gridWidth;
    const int   H = m_params.gridHeight;
    const float f = m_params.noiseFreq;

    for (int z = 0; z < H; ++z) {
        for (int x = 0; x < W; ++x) {
            float nx = (float)x * f;
            float nz = (float)z * f;

            float raw = m_noise.warpedFractal2D(
                nx, nz, m_params.octaves, m_params.warpStrength);

            float coast   = coastProfile((float)x / (float)(W - 1));
            float blended = raw * 0.5f + coast * 0.5f;
            float height  = cliffProfile(blended);

            m_heightmap->set(x, z, height * m_params.heightScale);
        }
    }

    m_heightmap->uploadToGPU();
}

void TerrainGenerator::buildMesh() {
    const int   W     = m_params.gridWidth;
    const int   H     = m_params.gridHeight;
    const float scale = m_params.worldScale / (float)(W - 1);

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
                px, py, pz,
                n.x, n.y, n.z,
                u, v
            });
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve((W - 1) * (H - 1) * 6);

    for (int z = 0; z < H - 1; ++z) {
        for (int x = 0; x < W - 1; ++x) {
            unsigned int tl = z * W + x;
            unsigned int tr = tl + 1;
            unsigned int bl = tl + W;
            unsigned int br = bl + 1;

            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }

    m_mesh->upload(vertices, indices);
}
