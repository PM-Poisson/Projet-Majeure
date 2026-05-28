#include "tree_system.hpp"
#include <random>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>

// ===========================================================================
// Construction / destruction
// ===========================================================================
TreeSystem::TreeSystem() {}

TreeSystem::~TreeSystem() {
    if (m_vao)     glDeleteVertexArrays(1, &m_vao);
    if (m_vbo)     glDeleteBuffers(1, &m_vbo);
    if (m_ebo)     glDeleteBuffers(1, &m_ebo);
    if (m_instVbo) glDeleteBuffers(1, &m_instVbo);
}

void TreeSystem::init() {
    m_shader.load("shaders/tree.vert", "shaders/tree.frag");
    buildBaseMesh();
}

// ===========================================================================
// Construction du mesh "sapin" (tronc + 3 cônes empilés)
//
// Layout par vertex : position(3) + normal(3) + color(3) = 9 floats
// On crée un seul mesh combiné avec les couleurs encodées par vertex
// → un seul draw call par instance d'arbre
// ===========================================================================
void TreeSystem::buildBaseMesh() {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    auto addVertex = [&](glm::vec3 p, glm::vec3 n, glm::vec3 c) {
        vertices.insert(vertices.end(),
            { p.x, p.y, p.z,  n.x, n.y, n.z,  c.x, c.y, c.z });
    };
    auto vertexCount = [&]() { return (int)(vertices.size() / 9); };

    // --- Constantes ---
    const int   trunkSeg   = 8;
    const float trunkR     = 0.07f;
    const float trunkH     = 0.6f;
    const glm::vec3 barkColor = glm::vec3(0.38f, 0.24f, 0.14f);

    const int   coneSeg     = 8;
    // Trois cônes empilés, rayons décroissants, formant un sapin
    //   niveau i : anneau de base à y = yBot[i], pointe à y = yBot[i+1]
    const float coneRadii[3] = { 0.55f, 0.45f, 0.32f };
    const float coneYs   [4] = { trunkH + 0.0f,
                                  trunkH + 0.55f,
                                  trunkH + 1.00f,
                                  trunkH + 1.55f };
    const glm::vec3 leafColor = glm::vec3(0.20f, 0.45f, 0.18f);

    // === Tronc (cylindre fermé latéralement, ouvert en haut/bas — masqué par le sol et le feuillage) ===
    int trunkBase = vertexCount();
    for (int i = 0; i <= trunkSeg; ++i) {
        float a  = float(i) / float(trunkSeg) * 6.28318530718f;
        float cx = std::cos(a), sz = std::sin(a);
        glm::vec3 normal(cx, 0.0f, sz);
        addVertex({cx * trunkR, 0.0f,   sz * trunkR}, normal, barkColor);
        addVertex({cx * trunkR, trunkH, sz * trunkR}, normal, barkColor);
    }
    for (int i = 0; i < trunkSeg; ++i) {
        unsigned int a = trunkBase + i * 2;
        unsigned int b = a + 1;
        unsigned int c = a + 2;
        unsigned int d = a + 3;
        indices.push_back(a); indices.push_back(c); indices.push_back(b);
        indices.push_back(b); indices.push_back(c); indices.push_back(d);
    }

    // === Feuillage : 3 cônes ===
    for (int level = 0; level < 3; ++level) {
        int   baseIdx = vertexCount();
        float r       = coneRadii[level];
        float yBot    = coneYs[level];
        float yTop    = coneYs[level + 1];

        // Anneau bas du cône
        for (int i = 0; i < coneSeg; ++i) {
            float a  = float(i) / float(coneSeg) * 6.28318530718f;
            float cx = std::cos(a), sz = std::sin(a);
            // Normale orientée vers l'extérieur, avec une composante Y positive
            glm::vec3 n = glm::normalize(glm::vec3(cx, 0.4f, sz));
            addVertex({cx * r, yBot, sz * r}, n, leafColor);
        }
        // Sommet du cône
        addVertex({0.0f, yTop, 0.0f}, glm::vec3(0, 1, 0), leafColor);
        int apex = baseIdx + coneSeg;

        for (int i = 0; i < coneSeg; ++i) {
            unsigned int a = baseIdx + i;
            unsigned int b = baseIdx + (i + 1) % coneSeg;
            indices.push_back(a); indices.push_back(b); indices.push_back(apex);
        }
    }

    m_indexCount = (int)indices.size();

    // === Création des buffers OpenGL ===
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glGenBuffers(1, &m_instVbo);

    glBindVertexArray(m_vao);

    // VBO mesh statique
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    const GLsizei stride = 9 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);                       // aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));     // aNormal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));     // aColor
    glEnableVertexAttribArray(2);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // VBO instances (vide pour l'instant, rempli dans uploadInstances)
    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    const GLsizei iStride = 8 * sizeof(float);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, iStride, (void*)0);                       // iPos
    glEnableVertexAttribArray(3); glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, iStride, (void*)(3 * sizeof(float)));     // iScale
    glEnableVertexAttribArray(4); glVertexAttribDivisor(4, 1);
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, iStride, (void*)(4 * sizeof(float)));     // iTint
    glEnableVertexAttribArray(5); glVertexAttribDivisor(5, 1);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, iStride, (void*)(7 * sizeof(float)));     // iRot
    glEnableVertexAttribArray(6); glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

// ===========================================================================
// Placement procédural des arbres
//
// Algorithme :
//   1. Parcours d'une grille espacée (params.gridSpacing) sur tout le terrain
//   2. Pour chaque case : position jitterée aléatoirement
//   3. Filtres :
//        a. hauteur dans [seaLevel + minHeight, maxHeight]
//        b. pente acceptable (normale.y > maxSlopeCos)
//        c. probabilité = densityNoise(position) * density  → clairières
//   4. Test de distance minimale via grille hashée (O(1) amorti)
//   5. Si OK : créer l'instance avec variations aléatoires (échelle/teinte/rotation)
// ===========================================================================
void TreeSystem::generate(const std::vector<float>& heights, int N,
                          float worldSize, float seaLevel,
                          uint32_t seed,
                          const TreePlacementParams& p)
{
    m_instances.clear();
    params = p;

    if (!p.enabled) {
        uploadInstances();
        return;
    }

    NoiseGenerator densityNoise(seed + 1000u);
    NoiseGenerator tintNoise   (seed + 2000u);
    std::mt19937   rng(seed);
    std::uniform_real_distribution<float> jit(-0.5f, 0.5f);
    std::uniform_real_distribution<float> uni(0.0f, 1.0f);

    const float half     = worldSize * 0.5f;
    const float cellSize = worldSize / float(N - 1);

    // --- Helpers d'interpolation ---
    auto sampleH = [&](float wx, float wz) -> float {
        float gx = (wx + half) / cellSize;
        float gz = (wz + half) / cellSize;
        int xi = std::clamp(int(gx), 0, N - 2);
        int zi = std::clamp(int(gz), 0, N - 2);
        float tx = gx - xi, tz = gz - zi;
        float h00 = heights[zi      * N + xi    ];
        float h10 = heights[zi      * N + xi + 1];
        float h01 = heights[(zi+1)  * N + xi    ];
        float h11 = heights[(zi+1)  * N + xi + 1];
        return (h00*(1-tx) + h10*tx)*(1-tz) + (h01*(1-tx) + h11*tx)*tz;
    };
    auto sampleNormalY = [&](float wx, float wz) -> float {
        float h0 = sampleH(wx,            wz);
        float hX = sampleH(wx + cellSize, wz);
        float hZ = sampleH(wx,            wz + cellSize);
        float dx = (hX - h0) / cellSize;
        float dz = (hZ - h0) / cellSize;
        // normale = normalize(-dx, 1, -dz) ; on retourne seulement Y
        return 1.0f / std::sqrt(dx*dx + 1.0f + dz*dz);
    };

    // --- Grille hashée pour la distance minimale ---
    const float minD    = p.minDistance;
    const float invMinD = 1.0f / minD;
    std::unordered_multimap<uint64_t, glm::vec2> hashGrid;

    auto cellKey = [invMinD](float x, float z) -> uint64_t {
        int cx = int(std::floor(x * invMinD));
        int cz = int(std::floor(z * invMinD));
        return ((uint64_t)(uint32_t)cx << 32) | (uint64_t)(uint32_t)cz;
    };
    auto canPlace = [&](float x, float z) {
        int cxBase = int(std::floor(x * invMinD));
        int czBase = int(std::floor(z * invMinD));
        for (int dz = -1; dz <= 1; ++dz)
        for (int dx = -1; dx <= 1; ++dx) {
            uint64_t k = ((uint64_t)(uint32_t)(cxBase+dx) << 32)
                       | (uint64_t)(uint32_t)(czBase+dz);
            auto range = hashGrid.equal_range(k);
            for (auto it = range.first; it != range.second; ++it) {
                float ddx = it->second.x - x;
                float ddz = it->second.y - z;
                if (ddx*ddx + ddz*ddz < minD * minD) return false;
            }
        }
        return true;
    };

    // --- Parcours principal ---
    // On exprime la grille en unités monde directement
    const float worldStep = p.gridSpacing;
    const float jitterAmp = p.jitterFactor * worldStep * 0.5f;

    int candidates = 0, accepted = 0;

    for (float gz = -half + worldStep; gz < half - worldStep; gz += worldStep) {
        for (float gx = -half + worldStep; gx < half - worldStep; gx += worldStep) {
            ++candidates;

            float wx = gx + jit(rng) * jitterAmp * 2.0f;
            float wz = gz + jit(rng) * jitterAmp * 2.0f;

            // (a) Filtre hauteur
            float h = sampleH(wx, wz);
            if (h < seaLevel + p.minHeight) continue;
            if (h > p.maxHeight)            continue;

            // (b) Filtre pente
            float ny = sampleNormalY(wx, wz);
            if (ny < p.maxSlopeCos)         continue;

            // (c) Filtre densité (bruit basse fréquence → clairières)
            float dens = densityNoise.warpedFractal2D(
                            wx * p.densityFreq, wz * p.densityFreq, 3, 0.0f);
            // Rehaussement du contraste : on décale le bruit vers le bas pour
            // qu'une large portion (~40%) soit en dessous de zéro → clairière franche.
            // Puis on applique une puissance pour rendre la transition forêt/clairière
            // plus abrupte (bordure nette plutôt que dégradé flou).
            float densShifted = (dens + 1.0f) * 0.5f - p.clearingBias; // décalage vers le bas
            float densContrast = std::pow(std::max(0.0f, densShifted), p.clearingContrast);
            float prob = densContrast * p.density;
            if (uni(rng) > prob) continue;

            // (d) Distance minimale
            if (!canPlace(wx, wz)) continue;

            // --- Création de l'instance ---
            TreeInstance inst;
            // Léger enfoncement dans le sol pour éviter le "flottement"
            inst.position = glm::vec3(wx, h - 0.05f, wz);
            inst.scale    = p.minScale + uni(rng) * (p.maxScale - p.minScale);
            inst.rotation = uni(rng) * 6.28318530718f;

            // Teinte du feuillage : interpolation vert-clair / jaunâtre via bruit
            float t = (tintNoise.warpedFractal2D(wx * 0.05f, wz * 0.05f, 2, 0.0f)
                       + 1.0f) * 0.5f;
            inst.foliageTint = glm::mix(
                glm::vec3(0.85f, 1.05f, 0.85f),   // vert frais
                glm::vec3(1.10f, 1.00f, 0.70f),   // vert jaunâtre
                t);

            m_instances.push_back(inst);
            hashGrid.insert({cellKey(wx, wz), glm::vec2(wx, wz)});
            ++accepted;
        }
    }

    std::cout << "[Trees] " << accepted << " / " << candidates
              << " candidats acceptés\n";

    uploadInstances();
}

// ===========================================================================
void TreeSystem::uploadInstances() {
    if (m_instances.empty()) {
        // Vider quand même le buffer pour rester cohérent
        glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
        return;
    }

    std::vector<float> data;
    data.reserve(m_instances.size() * 8);
    for (const auto& i : m_instances) {
        data.push_back(i.position.x);
        data.push_back(i.position.y);
        data.push_back(i.position.z);
        data.push_back(i.scale);
        data.push_back(i.foliageTint.r);
        data.push_back(i.foliageTint.g);
        data.push_back(i.foliageTint.b);
        data.push_back(i.rotation);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float),
                 data.data(), GL_STATIC_DRAW);
}

// ===========================================================================
void TreeSystem::draw(const glm::mat4& view, const glm::mat4& proj,
                     const glm::vec3& lightDir, const glm::vec3& cameraPos) {
    if (m_instances.empty()) return;

    m_shader.use();
    m_shader.setMat4("uView",       view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setVec3("uLightDir",   lightDir);
    m_shader.setVec3("uCameraPos",  cameraPos);

    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT,
                            nullptr, (GLsizei)m_instances.size());
    glBindVertexArray(0);
    m_shader.unuse();
}