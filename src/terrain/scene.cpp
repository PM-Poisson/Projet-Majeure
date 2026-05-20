#include "scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

IslandScene::IslandScene() : m_noise(42) {}

// ---------------------------------------------------------------------------
// radialCoastProfile
//
// Equivalent radial du coastProfile() de TerrainGenerator.
// d : distance normalisee au centre de l'ile, dans [0, 1]
//     0 = centre de l'ile, 1 = bord de la grille
//
// Retourne dans [-1, 1] :
//   > 0  => zone terrestre (plateau)
//   < 0  => zone marine (sous l'eau)
//
// La transition se fait autour de islandRadius, sur une largeur shoreBlend.
// Un shoreBlend petit (<0.08) donne des falaises tres abruptes.
// Un shoreBlend grand (>0.2) donne une plage en pente douce.
// ---------------------------------------------------------------------------
float IslandScene::radialCoastProfile(float d) const {
    // On centre la sigmoide sur islandRadius
    float x = (d - params.islandRadius) / params.shoreBlend;
    // tanh : vaut +1 loin du bord (centre ile), -1 loin du centre (ocean)
    // On l'inverse car d=0 est le centre et d=1 est le bord
    return -std::tanh(x * 2.5f);
}
// ---------------------------------------------------------------------------
// cliffProfile
//
// Repris identiquement de TerrainGenerator::cliffProfile().
// Aplatit les sommets (plateau) et les fonds (fond marin),
// mais rend la transition tres abrupte => effet falaise.
// ---------------------------------------------------------------------------
float IslandScene::cliffProfile(float rawHeight) const {
    float h = rawHeight;
    if (h > 0.0f)
        h = std::pow(h, 0.7f);   // plateau : compression des hautes valeurs
    else
        h = -std::pow(-h, 0.6f); // fond marin : compression des basses valeurs
    return h;
}

// ---------------------------------------------------------------------------
// buildMesh
//
// Algorithme (meme logique que TerrainGenerator::buildHeightmap) :
//
//  1. Bruit fractal warpé  => raw dans [-1,1]  (detail de surface)
//  2. Masque radial        => coast dans [-1,1] (forme ile/ocean)
//  3. Melange 50/50        => blended           (terrain brut)
//  4. cliffProfile         => height            (falaises accentuees)
//  5. Mise a l'echelle     => * heightScale
//
// La difference avec TerrainGenerator : le "profil de cote" est
// radial (distance au centre) plutot que lineaire (position en X).
// ---------------------------------------------------------------------------
void IslandScene::buildMesh() {
    const int   N    = params.gridN;
    const float half = params.worldSize * 0.5f;
    const float step = params.worldSize / float(N - 1);
    const float f    = params.noiseFreq;

    // --- Calcul des hauteurs CPU ---
    std::vector<float> heights(N * N);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            // Coordonnees normalisees [-1, 1]
            float nx = 2.0f * x / float(N - 1) - 1.0f;
            float nz = 2.0f * z / float(N - 1) - 1.0f;

            // Distance normalisee au centre [0, ~1.41]
            // On la normalise sur [0,1] en divisant par sqrt(2)
            float d = std::sqrt(nx * nx + nz * nz) / std::sqrt(2.0f);

            // 1. Bruit warpé (detail de surface du plateau et des falaises)
            //    Meme parametres que votre TerrainGenerator original
            float raw = m_noise.warpedFractal2D(
                nx / (f * 333.0f),   // conversion : noiseFreq ~0.003 => freq monde
                nz / (f * 333.0f),
                params.octaves,
                params.warpStrength
            );
            // raw dans [-1, 1]

            // 2. Masque radial (equivalent du coastProfile lineaire)
            float coast = radialCoastProfile(d);
            // coast dans [-1, 1] : +1 au centre, -1 en bordure

            // 3. Melange identique a TerrainGenerator :
            //    50% bruit + 50% forme => garde le detail tout en imposant la forme
            float blended = raw * 0.5f + coast * 0.5f;

            // 4. Profil de falaise : aplatit les hauts (plateau) et les bas (fond)
            float height = cliffProfile(blended);

            // 5. Mise a l'echelle
            heights[z * N + x] = height * params.heightScale;
        }
    }

    // --- Construction du mesh (position + normale + uv) ---
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N - 1) * (N - 1) * 6);

    // Acces securise a la heightmap pour les normales
    auto h = [&](int xi, int zi) -> float {
        xi = std::clamp(xi, 0, N - 1);
        zi = std::clamp(zi, 0, N - 1);
        return heights[zi * N + xi];
    };

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float py = heights[z * N + x];
            float pz = -half + z * step;

            // Normale par differences finies centrales (identique a Heightmap::getNormal)
            float hL = h(x-1, z), hR = h(x+1, z);
            float hD = h(x, z-1), hU = h(x, z+1);
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
    m_shader.setFloat("uSeaLevel",  params.seaLevel);
    m_shader.setVec3 ("uLightDir",  lightDir);
    m_shader.setVec3 ("uCameraPos",   cameraPos);
    m_mesh.draw();
    m_shader.unuse();
}