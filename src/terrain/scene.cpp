/**
 * scene.cpp — Génération procédurale de l'île côtière
 *
 * Ce fichier implémente IslandScene, qui orchestre la création du mesh
 * de l'île en deux temps :
 *   1. Génération d'une heightmap brute (Simplex ou Diamond-Square)
 *   2. Application d'un masque d'île radial asymétrique pour obtenir
 *      la forme finale avec plage et falaise
 */

#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

// Deux seeds différentes : une pour la surface, une pour la forme
IslandScene::IslandScene() : m_noise(42), m_noiseShape(137) {}

// ---------------------------------------------------------------------------
// localRadius — rayon effectif de l'île selon la position angulaire
//
// Un bruit fractal très basse fréquence (2 octaves, freq 0.4) est évalué
// en coordonnées normalisées. Il varie lentement autour de l'île, créant
// un côté avec un grand rayon (plage douce) et un autre plus petit (falaise).
// cliffiness contrôle l'amplitude de cette variation.
// ---------------------------------------------------------------------------
float IslandScene::localRadius(float nx, float nz) const {
    float shapeNoise = m_noiseShape.fractal2D(
        nx * 0.4f, nz * 0.4f,
        2,          // 2 octaves seulement : variation lente et simple
        2.0f, 0.5f
    );
    return params.islandRadius + shapeNoise * params.cliffiness;
}

// ---------------------------------------------------------------------------
// shoreProfile — profil de hauteur dans la zone de transition
//
// distToEdge > 0 : à l'intérieur de l'île (vers le plateau)
// distToEdge < 0 : à l'extérieur (vers la mer)
// beachFactor    : 0 = falaise directe, 1 = plage large
//
// Trois sous-zones :
//   [-blend, -beachWidth] : transition mer → plage (pente douce)
//   [-beachWidth, +beachWidth] : plage (quasi-plate, proche Y=0)
//   [+beachWidth, +blend] : transition plage → plateau (sigmoïde rapide)
// ---------------------------------------------------------------------------
float IslandScene::shoreProfile(float distToEdge, float beachFactor) const {
    float blend = params.shoreBlend;

    // Hors de la zone de transition : valeurs extrêmes directes
    if (distToEdge < -blend) return -1.0f;
    if (distToEdge >  blend) return  1.0f;

    float t          = distToEdge / blend;         // [-1, 1] dans la zone
    float beachWidth = beachFactor * 0.55f;        // largeur relative de la plage

    if (t > -beachWidth && t < beachWidth) {
        // Zone de plage : légère pente vers la mer
        return std::clamp(t / (beachWidth * 3.5f), -0.25f, 0.25f);
    } else if (t >= beachWidth) {
        // Montée rapide vers le plateau (falaise intérieure)
        float s = (t - beachWidth) / (1.0f - beachWidth);
        return 0.25f + 0.75f * std::tanh(s * 3.5f);
    } else {
        // Descente douce vers la mer
        float s = (t + beachWidth) / beachWidth;
        return -1.0f + 0.75f * std::tanh(s * 2.0f);
    }
}

// ---------------------------------------------------------------------------
// cliffProfile — accentue les falaises
//
// Applique une courbe en S sur la hauteur brute :
//   - Valeurs positives (terre) : exposant < 1 => aplatit les sommets
//   - Valeurs négatives (mer)   : exposant < 1 => aplatit les fonds
// La transition autour de 0 reste abrupte => effet falaise.
// ---------------------------------------------------------------------------
float IslandScene::cliffProfile(float rawHeight) const {
    float h = rawHeight;
    if (h > 0.0f) h =  std::pow( h, 0.72f);   // aplatit le plateau
    else           h = -std::pow(-h, 0.55f);   // aplatit le fond marin
    return h;
}

// ---------------------------------------------------------------------------
// applyIslandMask — transforme une heightmap brute en île
//
// Pour chaque point de la grille :
//   1. Calcul de la distance normalisée au centre (d ∈ [0, 1])
//   2. Rayon local selon l'angle (asymétrie falaise/plage)
//   3. Distance signée au bord → profil de rivage
//   4. beachFactor : côté grand rayon = plage, petit rayon = falaise
//   5. Mélange bruit/forme : le bruit domine au centre, la forme aux bords
//   6. cliffProfile + mise à l'échelle
// ---------------------------------------------------------------------------
std::vector<float> IslandScene::applyIslandMask(
        const std::vector<float>& rawHeights, int N) const
{
    std::vector<float> heights(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            // Coordonnées normalisées [-1, 1]
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            // Distance au centre normalisée dans [0, 1]
            float d      = std::sqrt(nx*nx + nz*nz) / std::sqrt(2.0f);
            float raw    = rawHeights[z * N + x];   // bruit brut [-1, 1]
            float radius = localRadius(nx, nz);      // rayon local asymétrique

            // Distance signée au bord de l'île
            float distToEdge  = (radius - d) / params.shoreBlend;
            float distClamped = std::clamp(distToEdge, -1.0f, 1.0f) * params.shoreBlend;

            // beachFactor : 0 si côté falaise, 1 si côté plage
            float radiusDev  = (radius - params.islandRadius)
                               / std::max(params.cliffiness, 1e-4f);
            float beachFactor = std::clamp(radiusDev * 0.5f + 0.5f, 0.0f, 1.0f);

            float shore = shoreProfile(distClamped, beachFactor);

            // Pondération : le bruit domine au centre du plateau (70%),
            // la forme domine en bord de côte (65% forme)
            float centerWeight = std::clamp(distClamped / params.shoreBlend, 0.0f, 1.0f);
            float noiseWeight  = 0.35f + centerWeight * 0.35f;
            float shoreWeight  = 1.0f - noiseWeight;
            float blended      = raw * noiseWeight + shore * shoreWeight;

            heights[z * N + x] = cliffProfile(blended) * params.heightScale;
        }
    }
    return heights;
}

// ---------------------------------------------------------------------------
// generateSimplex — heightmap brute par bruit Simplex + domain warping
//
// warpedFractal2D évalue d'abord un fBm pour obtenir un vecteur de
// déformation, puis évalue un second fBm dans l'espace déformé.
// Cela produit des formes très organiques (méandres, irrégularités).
// ---------------------------------------------------------------------------
std::vector<float> IslandScene::generateSimplex(int N) const {
    const float f = params.noiseFreq;
    std::vector<float> raw(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;
            // Division par (f * 333) : convertit noiseFreq (~0.003) en fréquence monde
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

// ---------------------------------------------------------------------------
// generateDiamondSquare — heightmap brute par subdivision récursive
//
// Génère d'abord à la résolution native (2^steps+1)², puis redimensionne
// par interpolation bilinéaire vers la résolution gridN.
// ---------------------------------------------------------------------------
std::vector<float> IslandScene::generateDiamondSquare() const {
    DiamondSquareParams dsp;
    dsp.steps     = params.dsSteps;
    dsp.roughness = params.dsRoughness;
    dsp.seed      = params.seed;

    std::vector<float> ds  = DiamondSquare::generate(dsp);
    int                dsN = DiamondSquare::gridSize(params.dsSteps);
    int                N   = params.gridN;

    std::vector<float> raw(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            // Coordonnées dans la grille DS (peut être fractionnaire)
            float fx = float(x) / float(N - 1) * float(dsN - 1);
            float fz = float(z) / float(N - 1) * float(dsN - 1);

            int x0 = std::clamp(int(fx),     0, dsN - 1);
            int x1 = std::clamp(int(fx) + 1, 0, dsN - 1);
            int z0 = std::clamp(int(fz),     0, dsN - 1);
            int z1 = std::clamp(int(fz) + 1, 0, dsN - 1);

            float tx = fx - float(x0);
            float tz = fz - float(z0);

            // Interpolation bilinéaire des 4 voisins
            float h00 = ds[z0 * dsN + x0];
            float h10 = ds[z0 * dsN + x1];
            float h01 = ds[z1 * dsN + x0];
            float h11 = ds[z1 * dsN + x1];

            raw[z * N + x] = (h00*(1-tx) + h10*tx) * (1-tz)
                           + (h01*(1-tx) + h11*tx) *    tz;
        }
    }
    return raw;
}

// ---------------------------------------------------------------------------
// buildMesh — construit le VAO/VBO/EBO OpenGL
//
// Format de chaque sommet : position(3) + normale(3) + uv(2) = 8 floats
// Les normales sont calculées par différences finies centrales sur la heightmap.
// La grille est triangulée en deux triangles par cellule (N-1)².
// ---------------------------------------------------------------------------
void IslandScene::buildMesh() {
    const int   N    = params.gridN;
    const float half = params.worldSize * 0.5f;
    const float step = params.worldSize / float(N - 1);

    // Génération de la heightmap brute selon la méthode choisie
    std::vector<float> raw;
    if (method == TerrainMethod::DiamondSquare)
        raw = generateDiamondSquare();
    else
        raw = generateSimplex(N);

    // Application du masque d'île (forme + profils)
    std::vector<float> heights = applyIslandMask(raw, N);

    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N-1) * (N-1) * 6);

    // Accès sécurisé à la heightmap (clamp aux bords pour les normales)
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

            // Normale par différences finies centrales
            // n = normalize( (hL-hR, 2*step, hD-hU) )
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

    // Triangulation : deux triangles par cellule (haut-gauche, bas-droite)
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

// ---------------------------------------------------------------------------
// API publique
// ---------------------------------------------------------------------------
void IslandScene::init(const IslandParams& p, TerrainMethod m) {
    params       = p;
    method       = m;
    m_noise      = NoiseGenerator(p.seed);
    m_noiseShape = NoiseGenerator(p.seed + 137); // seed décalée pour la forme
    m_shader.load("shaders/terrain.vert", "shaders/terrain.frag");
    buildMesh();
}

void IslandScene::regenerate(const IslandParams& p, TerrainMethod m) {
    params       = p;
    method       = m;
    m_noise      = NoiseGenerator(p.seed);
    m_noiseShape = NoiseGenerator(p.seed + 137);
    buildMesh();
}

void IslandScene::draw(const glm::mat4& view,
                       const glm::mat4& proj,
                       const glm::vec3& lightDir,
                       const glm::vec3& cameraPos)
{
    m_shader.use();
    m_shader.setMat4 ("uModel",       glm::mat4(1.0f));
    m_shader.setMat4 ("uView",        view);
    m_shader.setMat4 ("uProjection",  proj);
    m_shader.setFloat("uSeaLevel",    params.seaLevel);
    m_shader.setFloat("uHeightScale", params.heightScale);
    m_shader.setVec3 ("uLightDir",    lightDir);
    m_shader.setVec3 ("uCameraPos",   cameraPos);
    m_mesh.draw();
    m_shader.unuse();
}