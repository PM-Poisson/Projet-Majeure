#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

IslandScene::IslandScene() : m_noise(42), m_noiseShape(137) {}

// ===========================================================================
// Profils de forme (communs aux deux methodes)
// ===========================================================================

float IslandScene::localRadius(float nx, float nz) const {
    float shapeNoise = m_noiseShape.fractal2D(
        nx * 0.4f, nz * 0.4f, 2, 2.0f, 0.5f);
    return params.islandRadius + shapeNoise * params.cliffiness;
}

float IslandScene::shoreProfile(float distToEdge, float beachFactor) const {
    float blend = params.shoreBlend;
    if (distToEdge < -blend) return -1.0f;
    if (distToEdge >  blend) return  1.0f;

    float t = distToEdge / blend;
    float beachWidth = beachFactor * 0.55f;

    if (t > -beachWidth && t < beachWidth) {
        return std::clamp(t / (beachWidth * 3.5f), -0.25f, 0.25f);
    } else if (t >= beachWidth) {
        float s = (t - beachWidth) / (1.0f - beachWidth);
        return 0.25f + 0.75f * std::tanh(s * 3.5f);
    } else {
        float s = (t + beachWidth) / beachWidth;
        return -1.0f + 0.75f * std::tanh(s * 2.0f);
    }
}

float IslandScene::cliffProfile(float rawHeight) const {
    float h = rawHeight;
    if (h > 0.0f) h =  std::pow( h, 0.72f);
    else           h = -std::pow(-h, 0.55f);
    return h;
}

// ===========================================================================
// applyIslandMask
//
// Prend une heightmap brute ([-1,1], taille NxN) et applique :
//   1. Masque d'ile radial asymetrique (localRadius + shoreProfile)
//   2. cliffProfile
//   3. Mise a l'echelle par heightScale
//
// Appelee par les deux methodes de generation.
// ===========================================================================
std::vector<float> IslandScene::applyIslandMask(
        const std::vector<float>& rawHeights, int N) const
{
    std::vector<float> heights(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;
            float d  = std::sqrt(nx*nx + nz*nz) / std::sqrt(2.0f);

            float raw    = rawHeights[z * N + x];  // dans [-1, 1]
            float radius = localRadius(nx, nz);

            float distToEdge    = (radius - d) / params.shoreBlend;
            float distClamped   = std::clamp(distToEdge, -1.0f, 1.0f)
                                  * params.shoreBlend;

            float radiusDev     = (radius - params.islandRadius)
                                  / std::max(params.cliffiness, 1e-4f);
            float beachFactor   = std::clamp(radiusDev * 0.5f + 0.5f, 0.0f, 1.0f);

            float shore = shoreProfile(distClamped, beachFactor);

            float centerWeight = std::clamp(distClamped / params.shoreBlend,
                                            0.0f, 1.0f);
            float noiseWeight  = 0.35f + centerWeight * 0.35f;
            float shoreWeight  = 1.0f - noiseWeight;
            float blended      = raw * noiseWeight + shore * shoreWeight;

            heights[z * N + x] = cliffProfile(blended) * params.heightScale;
        }
    }
    return heights;
}

// ===========================================================================
// generateSimplex — methode originale
// ===========================================================================
std::vector<float> IslandScene::generateSimplex(int N) const {
    const float f = params.noiseFreq;
    std::vector<float> raw(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;
            raw[z * N + x] = m_noise.warpedFractal2D(
                nx / (f * 333.0f),
                nz / (f * 333.0f),
                params.octaves,
                params.warpStrength
            );
        }
    }
    return raw;
}

// ===========================================================================
// generateDiamondSquare — methode par subdivision [Galin et al. 2019]
//
// L'algorithme genere une grille de taille (2^dsSteps + 1)^2.
// On redimensionne ensuite bilineairement si la resolution demandee
// (gridN) est differente.
// ===========================================================================
std::vector<float> IslandScene::generateDiamondSquare() const {
    DiamondSquareParams dsp;
    dsp.steps     = params.dsSteps;
    dsp.roughness = params.dsRoughness;
    dsp.seed      = params.seed;

    // Genere la heightmap brute a la resolution native Diamond-Square
    std::vector<float> ds = DiamondSquare::generate(dsp);
    int dsN = DiamondSquare::gridSize(params.dsSteps);

    // Redimensionnement bilineaire vers gridN x gridN
    int N = params.gridN;
    std::vector<float> raw(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            // Coordonnees dans la grille DS
            float fx = float(x) / float(N - 1) * float(dsN - 1);
            float fz = float(z) / float(N - 1) * float(dsN - 1);

            int x0 = std::clamp(int(fx),     0, dsN - 1);
            int x1 = std::clamp(int(fx) + 1, 0, dsN - 1);
            int z0 = std::clamp(int(fz),     0, dsN - 1);
            int z1 = std::clamp(int(fz) + 1, 0, dsN - 1);

            float tx = fx - float(x0);
            float tz = fz - float(z0);

            // Interpolation bilineaire
            float h00 = ds[z0 * dsN + x0];
            float h10 = ds[z0 * dsN + x1];
            float h01 = ds[z1 * dsN + x0];
            float h11 = ds[z1 * dsN + x1];

            raw[z * N + x] = (h00 * (1-tx) + h10 * tx) * (1-tz)
                           + (h01 * (1-tx) + h11 * tx) *    tz;
        }
    }
    return raw;
}

// ===========================================================================
// buildMesh — construit le mesh OpenGL depuis les hauteurs calculees
// ===========================================================================
void IslandScene::buildMesh() {
    const int   N    = params.gridN;
    const float half = params.worldSize * 0.5f;
    const float step = params.worldSize / float(N - 1);

    // 1. Generation de la heightmap brute selon la methode choisie
    std::vector<float> raw;
    if (method == TerrainMethod::DiamondSquare)
        raw = generateDiamondSquare();
    else
        raw = generateSimplex(N);

    // 2. Application du masque d'ile + profils (commun aux deux methodes)
    std::vector<float> heights = applyIslandMask(raw, N);

    // 3. Construction du mesh (position + normale + uv)
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N-1) * (N-1) * 6);

    auto h = [&](int xi, int zi) -> float {
        xi = std::clamp(xi, 0, N-1);
        zi = std::clamp(zi, 0, N-1);
        return heights[zi * N + xi];
    };

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float py = heights[z * N + x];
            float pz = -half + z * step;

            float hL = h(x-1,z), hR = h(x+1,z);
            float hD = h(x,z-1), hU = h(x,z+1);
            glm::vec3 n = glm::normalize(glm::vec3(
                (hL - hR) / (2.0f * step),
                1.0f,
                (hD - hU) / (2.0f * step)
            ));

            vertices.insert(vertices.end(), {
                px, py, pz,
                n.x, n.y, n.z,
                float(x) / float(N-1),
                float(z) / float(N-1)
            });
        }
    }

    for (int z = 0; z < N-1; ++z) {
        for (int x = 0; x < N-1; ++x) {
            unsigned int tl = z*N+x, tr = tl+1;
            unsigned int bl = tl+N,  br = bl+1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }

    m_mesh.upload(vertices, indices);
}

// ===========================================================================
// API publique
// ===========================================================================
void IslandScene::init(const IslandParams& p, TerrainMethod m) {
    params      = p;
    method      = m;
    m_noise     = NoiseGenerator(p.seed);
    m_noiseShape= NoiseGenerator(p.seed + 137);
    m_shader.load("shaders/terrain.vert", "shaders/terrain.frag");
    buildMesh();
}

void IslandScene::regenerate(const IslandParams& p, TerrainMethod m) {
    params      = p;
    method      = m;
    m_noise     = NoiseGenerator(p.seed);
    m_noiseShape= NoiseGenerator(p.seed + 137);
    buildMesh();
}

void IslandScene::draw(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& lightDir,
                       const glm::vec3& cameraPos)
{
    m_shader.use();
    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4 ("uModel",      model);
    m_shader.setMat4 ("uView",       view);
    m_shader.setMat4 ("uProjection", proj);
    m_shader.setFloat("uSeaLevel",   params.seaLevel);
    m_shader.setFloat("uHeightScale", params.heightScale);
    m_shader.setVec3 ("uLightDir",   lightDir);
    m_shader.setVec3 ("uCameraPos",  cameraPos);
    m_mesh.draw();
    m_shader.unuse();
}