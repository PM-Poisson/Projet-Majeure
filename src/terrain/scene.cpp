#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

IslandScene::IslandScene() : m_noise(42) {}

// ---------------------------------------------------------------------------
// Génération des hauteurs — approche "tout bruit"
//
// On abandonne les fonctions analytiques (tanh, smoothstep) qui imposaient
// une structure radiale rigide. À la place :
//
//   continentScore(x, z) = bruit_continental(x, z) + (1 - falloff(d))
//                          └─── forme organique ──┘   └─ boost central ─┘
//
// Le rivage est l'iso-zéro de continentScore : il émerge naturellement de
// la forme du bruit, sans aucune contrainte angulaire ou de symétrie. Le
// seul terme "non-bruit" est le falloff polynomial qui force l'île à être
// centrée (sinon le terrain serait infini avec des continents au hasard).
//
// La hauteur finale = score continental (forme globale)
//                   + bruit de relief (sur terre uniquement)
//                   + bruit haute fréquence de rugosité (sur terre)
//                   + bruit bathymétrique (sous l'eau uniquement)
//
// Caractéristiques émergentes :
//   - Côte parfaitement organique (forme du bruit continental)
//   - Falaises et plages mélangées sans variation imposée
//   - Aucun effet d'horloge (pas de bruit angulaire)
//   - Pente sous-marine vivante (bathymétrie)
//   - Pas de grandes surfaces lisses (bruit de rugosité HF)
// ---------------------------------------------------------------------------
void IslandScene::generateHeights() {
    const int   N = params.gridN;
    const float f = params.noiseFreq;

    m_heights.resize(N * N);

    const float peak           = params.heightScale;
    const float continentAmp   = peak * 0.60f;
    const float reliefAmp      = peak * 0.55f;
    const float roughnessAmp   = peak * 0.05f;
    const float bathyAmp       = peak * 0.12f;

    // Le slider "Abruption falaise" (shoreBlend) pilote l'exposant du falloff.
    // shoreBlend petit  → exposant grand → côte abrupte (falaises)
    // shoreBlend grand  → exposant petit → côte douce (plages)
    // Mapping linéaire sur [0.03, 0.30] → [4.0, 1.2]
    const float shoreSharpness = std::clamp(
        4.0f - (params.shoreBlend - 0.03f) / 0.27f * 2.8f,
        1.2f, 4.0f);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            float d    = std::sqrt(nx * nx + nz * nz);
            float dRel = d / params.islandRadius;

            // === 1. BRUIT CONTINENTAL (forme globale de l'île) ===
            // Deux échelles superposées → contour à plusieurs niveaux de détail
            float continentLF = m_noise.warpedFractal2D(
                nx * 0.8f + 12.3f, nz * 0.8f - 7.1f, 5, 0.5f);
            float continentMF = m_noise.warpedFractal2D(
                nx * 1.8f + 5.5f, nz * 1.8f + 8.2f, 3, 0.3f);
            float continent = continentLF * 0.7f + continentMF * 0.3f;

            // === 2. FALLOFF RADIAL POLYNOMIAL ===
            // Seule expression analytique de la distance. (1 - falloff) :
            //   d=0       → +1 (boost central)
            //   d=R       →  0 (rivage théorique, modulé par le bruit)
            //   d>>R      → très négatif (force la mer en bordure)
            float falloff = std::pow(std::max(0.0f, dRel), shoreSharpness);

            // === 3. SCORE CONTINENTAL ===
            // Le rivage est exactement l'iso-zéro de cette quantité
            float continentScore = continent + (1.0f - falloff);

            // === 4. BRUIT DE RELIEF (multi-octaves, contrôlé par l'UI) ===
            float relief = m_noise.warpedFractal2D(
                nx / (f * 333.0f),
                nz / (f * 333.0f),
                params.octaves,
                params.warpStrength);

            // === 5. BRUIT DE RUGOSITÉ HF ===
            // Faible amplitude, haute fréquence → casse les grandes surfaces
            // lisses sans dénaturer la silhouette
            float roughness = m_noise.warpedFractal2D(
                nx * 12.0f + 99.9f, nz * 12.0f - 44.4f, 3, 0.0f);

            // === 6. BRUIT BATHYMÉTRIQUE ===
            // Canyons et monts sous-marins
            float bathy = m_noise.warpedFractal2D(
                nx * 2.5f + 33.1f, nz * 2.5f - 25.8f, 3, 0.4f);

            // === 7. COMPOSITION ===
            // Bornes [0, 1] pour éviter une amplification non bornée du bruit
            // loin du rivage (sinon la bathy dominerait au large)
            float onLand = std::clamp( continentScore, 0.0f, 1.0f);
            float inSea  = std::clamp(-continentScore, 0.0f, 1.0f);

            float height = continentScore * continentAmp
                         + relief    * reliefAmp    * onLand
                         + roughness * roughnessAmp * onLand
                         + bathy     * bathyAmp     * inSea;

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