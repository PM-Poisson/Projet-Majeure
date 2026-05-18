#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

IslandScene::IslandScene() : m_noise(42) {}

// ---------------------------------------------------------------------------
// Masque radial sigmoide : 1.0 au centre, 0.0 hors du rayon
// nx, ny dans [-1, 1]
// ---------------------------------------------------------------------------
float IslandScene::islandMask(float nx, float ny) const {
    float d    = std::sqrt(nx * nx + ny * ny);
    float edge = (d - params.islandRadius) / params.shoreBlend;
    float mask = 1.0f - std::tanh(edge * 2.5f);
    return std::clamp(mask * 0.5f, 0.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// Construction du mesh
// Format : position(3) + normale(3) + uv(2) = 8 floats par sommet
// ---------------------------------------------------------------------------
void IslandScene::buildMesh() {
    const int   N    = params.gridN;
    const float half = params.worldSize * 0.5f;
    const float step = params.worldSize / float(N - 1);

    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N - 1) * (N - 1) * 6);

    // --- Calcul des hauteurs (CPU) ---
    std::vector<float> heights(N * N);
    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;  // [-1, 1]
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            float raw  = m_noise.warpedFractal2D(
                nx * params.noiseFreq * 1000.0f,
                nz * params.noiseFreq * 1000.0f,
                params.octaves,
                params.warpStrength
            );
            float mask = islandMask(nx, nz);
            heights[z * N + x] = raw * mask * params.heightScale;
        }
    }

    // --- Sommets avec normales par differences finies ---
    auto h = [&](int x, int z) -> float {
        x = std::clamp(x, 0, N - 1);
        z = std::clamp(z, 0, N - 1);
        return heights[z * N + x];
    };

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float py = heights[z * N + x];
            float pz = -half + z * step;

            // Normale par differences finies centrales
            float hL = h(x - 1, z), hR = h(x + 1, z);
            float hD = h(x, z - 1), hU = h(x, z + 1);
            glm::vec3 n = glm::normalize(glm::vec3(
                (hL - hR) / (2.0f * step),
                1.0f,
                (hD - hU) / (2.0f * step)
            ));

            float u = float(x) / float(N - 1);
            float v = float(z) / float(N - 1);

            vertices.insert(vertices.end(), {
                px, py, pz,
                n.x, n.y, n.z,
                u, v
            });
        }
    }

    // --- Indices ---
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

// ---------------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------------
void IslandScene::init(const IslandParams& p) {
    params  = p;
    m_noise = NoiseGenerator(p.seed);
    m_shader.load("shaders/terrain.vert", "shaders/terrain.frag");
    buildMesh();
}

void IslandScene::regenerate(const IslandParams& p) {
    params  = p;
    m_noise = NoiseGenerator(p.seed);
    buildMesh();
}

void IslandScene::draw(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& lightDir,
                       const glm::vec3& cameraPos)
{
    m_shader.use();

    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4 ("uModel",       model);
    m_shader.setMat4 ("uView",        view);
    m_shader.setMat4 ("uProjection",  proj);
    m_shader.setFloat("uSeaLevel",    0.0f);
    m_shader.setFloat("uHeightScale", params.heightScale);
    m_shader.setVec3 ("uLightDir",    lightDir);
    m_shader.setVec3 ("uCameraPos",   cameraPos);

    m_mesh.draw();
    m_shader.unuse();
}