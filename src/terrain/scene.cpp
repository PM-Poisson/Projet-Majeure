#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

IslandScene::IslandScene() : m_noise(42) {}

float IslandScene::radialCoastProfile(float d, float localBlend) const {
    float x = (d - params.islandRadius) / localBlend;
    return -std::tanh(x * 2.5f);
}

float IslandScene::cliffProfile(float rawHeight) const {
    float h = rawHeight;
    if (h > 0.0f)
        h = std::pow(h, 0.7f);
    else
        h = -std::pow(-h, 0.6f);
    return h;
}

// ---------------------------------------------------------------------------
// Génération des hauteurs
//
// Nouveauté : shoreBlend varie angulairement via un bruit basse fréquence.
//   - zones à blend faible  (→ params.shoreBlend * 0.3) : falaises abruptes
//   - zones à blend élevé   (→ params.shoreBlend * 3.0) : plages en pente douce
//
// Le bruit de variation est échantillonné sur un cercle unitaire (cos/sin)
// pour éviter toute discontinuité angulaire.
// ---------------------------------------------------------------------------
void IslandScene::generateHeights() {
    const int   N = params.gridN;
    const float f = params.noiseFreq;

    m_heights.resize(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            float d     = std::sqrt(nx * nx + nz * nz) / std::sqrt(2.0f);
            float angle = std::atan2(nz, nx);   // [-π, π]

            // --- Bruit angulaire pour la variation de shore ---
            // On sample sur un cercle pour avoir une périodicité parfaite en 2π.
            // Fréquence basse (÷8) : 2-4 "zones" distinctes autour de l'île.
            float freq  = 1.8f;
            float sampleX = std::cos(angle) * freq;
            float sampleZ = std::sin(angle) * freq;
            // Bruit simple (1 octave, pas de warp) pour un contour naturel
            float shoreNoise = m_noise.warpedFractal2D(sampleX, sampleZ, 2, 0.0f);
            // shoreNoise ∈ [-1, 1]

            // Remappage [−1,1] → [blendMin, blendMax]
            float blendMin = params.shoreBlend * 0.8f;   // falaise modérée
            float blendMax = params.shoreBlend * 5.0f;   // plage très douce
            float t = (shoreNoise + 1.0f) * 0.5f;        // → [0, 1]
            float localBlend = blendMin + t * (blendMax - blendMin);

            // --- Algorithme original ---
            float raw = m_noise.warpedFractal2D(
                nx / (f * 333.0f),
                nz / (f * 333.0f),
                params.octaves,
                params.warpStrength);

            float coast   = radialCoastProfile(d, localBlend);
            float blended = raw * 0.5f + coast * 0.5f;
            float height  = cliffProfile(blended);

            m_heights[z * N + x] = height * params.heightScale;
        }
    }
}

void IslandScene::applyErosion() {
    params.erosion.rain.seaLevel = params.seaLevel;

    std::cout << "[Erosion] " << params.gridN << "x" << params.gridN
              << "  gouttes=" << params.erosion.rain.numDroplets
              << "  steps="   << params.erosion.rain.maxSteps << "\n";

    ErosionSystem sys;
    sys.apply(m_heights, params.gridN, params.worldSize,
              params.erosion, params.seed + 1);

    std::cout << "[Erosion] Terminé.\n";
}

void IslandScene::buildMesh() {
    const int   N    = params.gridN;
    const float half = params.worldSize * 0.5f;
    const float step = params.worldSize / float(N - 1);

    auto h = [&](int xi, int zi) -> float {
        xi = std::clamp(xi, 0, N - 1);
        zi = std::clamp(zi, 0, N - 1);
        return m_heights[zi * N + xi];
    };

    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N - 1) * (N - 1) * 6);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float py = m_heights[z * N + x];
            float pz = -half + z * step;

            float hL = h(x-1,z), hR = h(x+1,z);
            float hD = h(x,z-1), hU = h(x,z+1);
            glm::vec3 n = glm::normalize(glm::vec3(
                (hL - hR) / (2.0f * step),
                1.0f,
                (hD - hU) / (2.0f * step)));

            float u = float(x) / float(N - 1);
            float v = float(z) / float(N - 1);

            vertices.insert(vertices.end(), {
                px, py, pz,
                n.x, n.y, n.z,
                u, v
            });
        }
    }

    for (int z = 0; z < N - 1; ++z) {
        for (int x = 0; x < N - 1; ++x) {
            unsigned int tl = z * N + x;
            unsigned int tr = tl + 1;
            unsigned int bl = tl + N;
            unsigned int br = bl + 1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }

    m_mesh.upload(vertices, indices);
}

void IslandScene::init(const IslandParams& p) {
    params  = p;
    m_noise = NoiseGenerator(p.seed);
    m_shader.load("shaders/terrain.vert", "shaders/terrain.frag");
    generateHeights();
    if (params.erosion.enabled) applyErosion();
    buildMesh();
}

void IslandScene::regenerate(const IslandParams& p) {
    params  = p;
    m_noise = NoiseGenerator(p.seed);
    generateHeights();
    if (params.erosion.enabled) applyErosion();
    buildMesh();
}

void IslandScene::draw(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& lightDir,
                       const glm::vec3& cameraPos) {
    m_shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4 ("uModel",     model);
    m_shader.setMat4 ("uView",      view);
    m_shader.setMat4 ("uProjection",proj);
    m_shader.setFloat("uSeaLevel",  params.seaLevel);
    m_shader.setVec3 ("uLightDir",  lightDir);
    m_shader.setVec3 ("uCameraPos", cameraPos);
    m_mesh.draw();
    m_shader.unuse();
}
