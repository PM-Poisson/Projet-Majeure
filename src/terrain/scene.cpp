#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

IslandScene::IslandScene() : m_noise(42) {}

// ---------------------------------------------------------------------------
// Génération des hauteurs — approche "baseline bathymétrique + bruits"
//
// L'ancienne approche faisait croître la profondeur de manière non bornée
// (`pow(dRel, k)`), ce qui créait un grand cône sous l'eau. Ici on borne :
//
//   bathymetricBase(d) = peak  ──────╮
//                                     ╲
//                                      ╲   transition smoothstep
//                                       ╲
//                       0  ────────────── ╲ ── rivage théorique
//                                          ╲
//                                           ╲
//                    -abyssDepth ──────────── ──────────── (plat au large)
//
// Cette baseline est ensuite :
//   - déformée horizontalement par un bruit continental → côte organique
//   - augmentée par un bruit de relief sur la terre
//   - augmentée par un bruit bathymétrique modeste sous l'eau
//
// Caractéristiques émergentes :
//   - Sol sous-marin plat à -abyssDepth (avec du bruit), pas de cône
//   - Remontée progressive vers l'île via smoothstep
//   - Côte organique (déformation du d par bruit continental)
//   - Plus de grandes surfaces lisses (bruit de rugosité HF)
// ---------------------------------------------------------------------------
void IslandScene::generateHeights() {
    const int   N = params.gridN;
    const float f = params.noiseFreq;

    m_heights.resize(N * N);

    const float peak           = params.heightScale;
    const float reliefAmp      = peak * 0.55f;
    const float roughnessAmp   = peak * 0.05f;
    const float bathyAmp       = peak * 0.08f;          // ~3 unités si peak=35
    const float abyssDepth     = peak * 0.45f;          // ~16 unités si peak=35

    const float islandR        = params.islandRadius;

    // shoreBlend pilote la largeur de la transition smoothstep mer↔terre :
    //   shoreBlend petit  → transition étroite → falaise abrupte
    //   shoreBlend grand  → transition large   → plage douce
    const float transitionHalf = islandR * (0.30f + params.shoreBlend * 2.0f);
    const float tStart = islandR - transitionHalf;
    const float tRange = 2.0f * transitionHalf;

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            float d = std::sqrt(nx * nx + nz * nz);

            // === 1. DÉFORMATION DU CONTOUR ===
            // Le rivage suit l'iso-d_eff, donc déformer d_eff par un bruit 2D
            // produit une côte parfaitement organique (baies, péninsules)
            float continentLF = m_noise.warpedFractal2D(
                nx * 0.8f + 12.3f, nz * 0.8f - 7.1f, 5, 0.5f);
            float continentMF = m_noise.warpedFractal2D(
                nx * 1.8f + 5.5f, nz * 1.8f + 8.2f, 3, 0.3f);
            float continent = continentLF * 0.7f + continentMF * 0.3f;
            float d_eff = d - continent * islandR * 0.25f;

            // === 2. BASELINE BATHYMÉTRIQUE (BORNÉE) ===
            // smoothstep(tStart, tStart+tRange, d_eff) : 0 sur l'île, 1 dans les abysses
            float t = std::clamp((d_eff - tStart) / tRange, 0.0f, 1.0f);
            t = t * t * (3.0f - 2.0f * t);                // smoothstep cubique
            float baselineH = peak - (peak + abyssDepth) * t;
            //   d_eff = tStart     : t=0 → baselineH = peak
            //   d_eff = tStart+R/2 : t=0.5 → baselineH = (peak - abyssDepth) / 2
            //   d_eff > tStart+R   : t=1 → baselineH = -abyssDepth (PLATEAU)

            // === 3. BRUITS DE DÉTAIL ===
            float relief = m_noise.warpedFractal2D(
                nx / (f * 333.0f), nz / (f * 333.0f),
                params.octaves, params.warpStrength);

            float roughness = m_noise.warpedFractal2D(
                nx * 12.0f + 99.9f, nz * 12.0f - 44.4f, 3, 0.0f);

            float bathy = m_noise.warpedFractal2D(
                nx * 2.5f + 33.1f, nz * 2.5f - 25.8f, 3, 0.4f);

            // === 4. MASQUES TERRE / MER selon la baseline ===
            // landiness : 0 dès qu'on est sous le niveau de la mer, 1 dès qu'on
            // est bien en altitude → le relief ne perturbe pas le bord du rivage
            float landiness  = std::clamp((baselineH + 2.0f) / (peak * 0.5f),
                                          0.0f, 1.0f);
            float oceaniness = 1.0f - landiness;

            // === 5. COMPOSITION ===
            float height = baselineH
                         + relief    * reliefAmp    * landiness
                         + roughness * roughnessAmp * landiness
                         + bathy     * bathyAmp     * oceaniness;

            m_heights[z * N + x] = height;
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
