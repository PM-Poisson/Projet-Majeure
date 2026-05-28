#include "fish_system.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>


// ---------------------------------------------------------------------------
// Reproduction CPU des vagues Gerstner du ocean.vert
// Doit rester synchronisée avec les paramètres du shader.
// ---------------------------------------------------------------------------
static const float OCEAN_BASE_Y = -3.5f;  // même valeur que ocean.hpp::oceanBaseY

static float gerstnerY(float px, float pz,
                        float amplitude, float wavelength,
                        float speed, float dirX, float dirZ,
                        float time)
{
    // Normalise la direction
    float len = std::sqrt(dirX*dirX + dirZ*dirZ);
    dirX /= len; dirZ /= len;

    float k = 6.28318530718f / wavelength;
    float f = k * (dirX*px + dirZ*pz) - speed * time;
    return amplitude * std::sin(f);
}

// Hauteur Y de la surface de l'eau au point (wx, wz) au temps t
// Inclut oceanBaseY (translation verticale du plan de base)
static float oceanSurfaceY(float wx, float wz, float t)
{
    float y = OCEAN_BASE_Y;
    y += gerstnerY(wx, wz, 1.8f,  80.0f, 1.5f,  1.0f,  0.3f, t);
    y += gerstnerY(wx, wz, 1.1f,  55.0f, 1.2f,  0.7f,  1.0f, t);
    y += gerstnerY(wx, wz, 0.6f,  32.0f, 0.9f, -0.4f,  0.8f, t);
    y += gerstnerY(wx, wz, 0.25f, 14.0f, 1.8f,  0.9f, -0.2f, t);
    y += gerstnerY(wx, wz, 0.15f,  9.0f, 2.2f, -0.6f, -0.7f, t);
    return y;
}

FishSystem::FishSystem() : m_rng(42) {}

FishSystem::~FishSystem() {
    if (m_vao)     glDeleteVertexArrays(1, &m_vao);
    if (m_vbo)     glDeleteBuffers(1, &m_vbo);
    if (m_ebo)     glDeleteBuffers(1, &m_ebo);
    if (m_instVbo) glDeleteBuffers(1, &m_instVbo);
}

// ===========================================================================
// Mesh du poisson : corps ellipsoïdal + nageoire caudale
//
// Orienté le long de +Z (direction de nage).
// Le vertex shader tourne l'instance pour aligner +Z sur la vélocité.
//
// Format par vertex : position(3) + normale(3) = 6 floats
// ===========================================================================
void FishSystem::buildFishMesh() {
    std::vector<float>        verts;
    std::vector<unsigned int> idx;

    auto addV = [&](glm::vec3 p, glm::vec3 n) {
        verts.insert(verts.end(), {p.x,p.y,p.z, n.x,n.y,n.z});
    };
    auto base = [&]() { return (unsigned int)(verts.size() / 6); };

    // --- Corps : ellipsoïde basse poly (2 demi-cônes)
    const int SEG = 8;
    const float bodyL  = 0.6f;   // demi-longueur
    const float bodyR  = 0.18f;  // rayon max (milieu)

    // Anneau central
    unsigned int ringBase = base();
    for (int i = 0; i < SEG; ++i) {
        float a = float(i) / SEG * 6.28318f;
        float cx = std::cos(a) * bodyR;
        float cy = std::sin(a) * bodyR;
        glm::vec3 n = glm::normalize(glm::vec3(cx, cy, 0.0f));
        addV({cx, cy, 0.0f}, n);
    }
    // Pointe avant (museau)
    unsigned int noseIdx = base();
    addV({0.0f, 0.0f, bodyL}, {0.0f, 0.0f, 1.0f});
    // Pointe arrière (queue)
    unsigned int tailIdx = base();
    addV({0.0f, 0.0f, -bodyL}, {0.0f, 0.0f, -1.0f});

    for (int i = 0; i < SEG; ++i) {
        unsigned int a = ringBase + i;
        unsigned int b = ringBase + (i+1) % SEG;
        // Avant
        idx.push_back(a); idx.push_back(b); idx.push_back(noseIdx);
        // Arrière
        idx.push_back(b); idx.push_back(a); idx.push_back(tailIdx);
    }

    // --- Nageoire caudale (triangle plat derrière la queue)
    float finW = 0.22f, finH = 0.12f, finZ = -bodyL - 0.15f;
    unsigned int fBase = base();
    addV({0.0f,       0.0f,    finZ},         {0.0f, 0.0f, -1.0f});
    addV({-finW,      finH*0.5f, finZ-0.18f}, {0.0f, 0.0f, -1.0f});
    addV({ finW,      finH*0.5f, finZ-0.18f}, {0.0f, 0.0f, -1.0f});
    addV({ 0.0f,     -finH,     finZ-0.12f},  {0.0f, 0.0f, -1.0f});

    idx.push_back(fBase);   idx.push_back(fBase+1); idx.push_back(fBase+2);
    idx.push_back(fBase);   idx.push_back(fBase+3); idx.push_back(fBase+1);
    idx.push_back(fBase);   idx.push_back(fBase+2); idx.push_back(fBase+3);

    m_indexCount = (int)idx.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glGenBuffers(1, &m_instVbo);

    glBindVertexArray(m_vao);

    // Mesh statique
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    const GLsizei stride = 6*sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);

    // VBO instances : mat4(16) + color(3) = 19 floats par instance
    // On passe une mat4 complète pour l'orientation (4 attributs vec4, locations 2-5)
    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    const GLsizei is = 19*sizeof(float);
    for (int col = 0; col < 4; ++col) {
        glVertexAttribPointer(2+col, 4, GL_FLOAT, GL_FALSE, is,
                              (void*)((col*4)*sizeof(float)));
        glEnableVertexAttribArray(2+col);
        glVertexAttribDivisor(2+col, 1);
    }
    glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, is, (void*)(16*sizeof(float)));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

// ===========================================================================
void FishSystem::init() {
    m_shader.load("shaders/fish.vert", "shaders/fish.frag");
    buildFishMesh();
}

// ===========================================================================
// Spawn : place les bancs en cercle autour de l'île, sous l'eau
// ===========================================================================
void FishSystem::spawn(const std::vector<float>& heights, int gridN,
                       float worldSize, float seaLevel,
                       uint32_t seed, const FishParams& p)
{
    m_heights   = &heights;
    m_gridN     = gridN;
    m_worldSize = worldSize;
    m_seaLevel  = seaLevel;
    params      = p;
    m_rng       = std::mt19937(seed);
    m_schools.clear();

    if (!p.enabled) { uploadInstances(); return; }

    std::uniform_real_distribution<float> angle(0.0f, 6.28318f);
    std::uniform_real_distribution<float> rad(worldSize*0.35f, worldSize*0.44f);
    std::uniform_real_distribution<float> depth(p.minDepth * 0.85f, p.maxDepth);
    std::uniform_real_distribution<float> vel(-1.0f, 1.0f);
    std::uniform_real_distribution<float> noiseOff(0.0f, 100.0f);

    // Couleurs des bancs
    const glm::vec3 schoolColors[] = {
        {0.8f, 0.5f, 0.15f},  // orange (poissons tropicaux)
        {0.3f, 0.6f, 0.9f},   // bleu-argent
        {0.6f, 0.9f, 0.4f},   // vert irisé
    };

    for (int s = 0; s < p.numSchools; ++s) {
        School school;
        school.color = schoolColors[s % 3];

        // Centre du banc : position aléatoire dans l'eau autour de l'île
        float a = angle(m_rng);
        float r = rad(m_rng);
        glm::vec3 schoolCenter(
            std::cos(a) * r,
            (p.minDepth + p.maxDepth) * 0.5f,
            std::sin(a) * r
        );

        for (int i = 0; i < p.fishPerSchool; ++i) {
            Fish f;
            // Position dispersée autour du centre du banc
            f.pos = schoolCenter + glm::vec3(
                vel(m_rng)*3.0f, vel(m_rng)*1.5f, vel(m_rng)*3.0f);

            // S'assurer qu'on est sous l'eau et au-dessus du terrain
            float th = terrainHeightAt(f.pos.x, f.pos.z);
            f.pos.y = std::clamp(f.pos.y, th + 1.0f, seaLevel - 0.5f);

            // Vélocité initiale aléatoire
            f.vel = glm::vec3(vel(m_rng), vel(m_rng)*0.3f, vel(m_rng)) * 2.0f;
            if (glm::length(f.vel) < 0.01f) f.vel = {1.0f, 0.0f, 0.0f};

            f.noiseOffset = noiseOff(m_rng);
            school.fish.push_back(f);
        }
        school.center = schoolCenter;
        m_schools.push_back(school);
    }

    std::cout << "[Fish] " << m_schools.size() << " bancs de "
              << p.fishPerSchool << " poissons\n";
    uploadInstances();
}

// ===========================================================================
// Hauteur terrain en position monde
// ===========================================================================
float FishSystem::terrainHeightAt(float wx, float wz) const {
    if (!m_heights || m_gridN == 0) return -999.0f;
    float half = m_worldSize * 0.5f;
    float cell = m_worldSize / float(m_gridN - 1);
    float gx = (wx + half) / cell;
    float gz = (wz + half) / cell;
    int x0 = std::clamp((int)gx, 0, m_gridN - 2);
    int z0 = std::clamp((int)gz, 0, m_gridN - 2);
    float tx = gx - x0, tz = gz - z0;
    float h00 = (*m_heights)[z0     * m_gridN + x0    ];
    float h10 = (*m_heights)[z0     * m_gridN + x0 + 1];
    float h01 = (*m_heights)[(z0+1) * m_gridN + x0    ];
    float h11 = (*m_heights)[(z0+1) * m_gridN + x0 + 1];
    return (h00*(1-tx)+h10*tx)*(1-tz) + (h01*(1-tx)+h11*tx)*tz;
}

// ===========================================================================
// Force Boids pour un poisson
// ===========================================================================
glm::vec3 FishSystem::computeBoidForce(const Fish& fish,
                                        const School& school,
                                        float noiseTime) const {
    glm::vec3 cohesion(0), alignment(0), separation(0);
    int cohCount = 0, sepCount = 0;

    for (const auto& other : school.fish) {
        if (&other == &fish) continue;
        glm::vec3 diff = other.pos - fish.pos;
        float dist = glm::length(diff);
        if (dist < 0.001f) continue;

        if (dist < params.cohesionRadius) {
            cohesion  += other.pos;
            alignment += other.vel;
            ++cohCount;
        }
        if (dist < params.separationRadius) {
            // Répulsion inversement proportionnelle à la distance
            separation -= glm::normalize(diff) / dist;
            ++sepCount;
        }
    }

    glm::vec3 force(0.0f);

    if (cohCount > 0) {
        cohesion  /= float(cohCount);
        cohesion   = cohesion - fish.pos;   // vecteur vers le centre du banc
        if (glm::length(cohesion) > 0.001f)
            force += glm::normalize(cohesion) * params.cohesionForce;

        alignment /= float(cohCount);
        if (glm::length(alignment) > 0.001f)
            force += glm::normalize(alignment) * params.alignmentForce;
    }

    if (sepCount > 0)
        force += separation * params.separationForce;

    // Bruit d'exploration : bruit pseudo-aléatoire lent basé sur le temps
    // Utilise des sin/cos décorrélés pour simuler un bruit smooth sans lib externe
    float nt = noiseTime + fish.noiseOffset;
    glm::vec3 explore(
        std::sin(nt * 0.7f  + fish.noiseOffset * 0.1f),
        std::cos(nt * 0.5f  + fish.noiseOffset * 0.2f) * 0.4f,
        std::sin(nt * 0.9f  + fish.noiseOffset * 0.15f + 1.3f)
    );
    force += explore * params.explorationForce;

    return force;
}

// ===========================================================================
// Force environnement (confinement vertical + évitement terrain)
// ===========================================================================
glm::vec3 FishSystem::computeEnvironmentForce(const Fish& fish) const {
    glm::vec3 force(0.0f);

    // --- Confinement vertical ---
    if (fish.pos.y < params.minDepth)
        force.y += (params.minDepth - fish.pos.y) * params.depthForce;
    // Plafond : surface de l'eau réelle au point courant
    float dynamicSurface = oceanSurfaceY(fish.pos.x, fish.pos.z, m_time) - 0.8f;
    if (fish.pos.y > dynamicSurface)
        force.y -= (fish.pos.y - dynamicSurface) * params.depthForce * 2.0f;

    // --- Évitement terrain ---
    // Échantillonne la hauteur du terrain juste en dessous du poisson
    float th = terrainHeightAt(fish.pos.x, fish.pos.z);
    float margin = params.terrainMargin;
    if (fish.pos.y < th + margin) {
        float penetration = (th + margin) - fish.pos.y;
        force.y += penetration * params.terrainForce;

        // Force horizontale : repousse latéralement en cherchant la direction
        // la plus libre (gradient de la hauteur terrain → s'éloigner de la colline)
        float dx = terrainHeightAt(fish.pos.x + 0.5f, fish.pos.z)
                 - terrainHeightAt(fish.pos.x - 0.5f, fish.pos.z);
        float dz = terrainHeightAt(fish.pos.x, fish.pos.z + 0.5f)
                 - terrainHeightAt(fish.pos.x, fish.pos.z - 0.5f);
        glm::vec3 grad(dx, 0.0f, dz);
        if (glm::length(grad) > 0.001f)
            force -= glm::normalize(grad) * penetration * params.terrainForce * 0.5f;
    }

    // --- Confinement horizontal (ne pas sortir de la zone eau) ---
    float border = m_worldSize * 0.45f;
    if (std::abs(fish.pos.x) > border)
        force.x -= std::copysign(params.depthForce * 2.0f, fish.pos.x);
    if (std::abs(fish.pos.z) > border)
        force.z -= std::copysign(params.depthForce * 2.0f, fish.pos.z);

    return force;
}

// ===========================================================================
// Update : simulation Boids + intégration Euler semi-implicite
// ===========================================================================
void FishSystem::update(float dt, const std::vector<float>& heights,
                         int gridN, float worldSize) {
    if (!params.enabled || m_schools.empty()) return;

    m_heights   = &heights;
    m_gridN     = gridN;
    m_worldSize = worldSize;
    m_time     += dt;

    for (auto& school : m_schools) {
        // Mise à jour du centre du banc
        school.center = glm::vec3(0.0f);
        for (const auto& f : school.fish) school.center += f.pos;
        school.center /= float(school.fish.size());

        // Mise à jour de chaque poisson
        for (auto& fish : school.fish) {
            glm::vec3 boid = computeBoidForce(fish, school, m_time);
            glm::vec3 env  = computeEnvironmentForce(fish);
            glm::vec3 acc  = boid + env;

            // Intégration semi-implicite (stable)
            fish.vel += acc * dt;

            // Clamp de la vitesse
            float speed = glm::length(fish.vel);
            if (speed > params.maxSpeed)
                fish.vel = fish.vel / speed * params.maxSpeed;
            else if (speed < params.minSpeed && speed > 0.001f)
                fish.vel = fish.vel / speed * params.minSpeed;

            fish.pos += fish.vel * dt;

            // Correction finale : si on est quand même dans la terre, remonter
            float th = terrainHeightAt(fish.pos.x, fish.pos.z);
            if (fish.pos.y < th + 0.3f) {
                fish.pos.y = th + 0.3f;
                fish.vel.y = std::abs(fish.vel.y) + 0.5f;
            }
            // Plafond dynamique : surface de l'eau réelle (vagues Gerstner)
            // On laisse une marge de 0.8 unités sous la surface visible
            float surfY = oceanSurfaceY(fish.pos.x, fish.pos.z, m_time) - 0.8f;
            if (fish.pos.y > surfY) {
                fish.pos.y = surfY;
                fish.vel.y = -std::abs(fish.vel.y);
            }
        }
    }

    uploadInstances();
}

// ===========================================================================
// Upload VBO instances : mat4 de transformation + couleur
// ===========================================================================
void FishSystem::uploadInstances() {
    std::vector<float> data;
    int total = totalFish();
    data.reserve(total * 19);

    for (const auto& school : m_schools) {
        for (const auto& fish : school.fish) {
            // Construire la matrice de rotation pour aligner +Z sur la vélocité
            glm::vec3 forward = fish.vel;
            float spd = glm::length(forward);
            if (spd < 0.001f) forward = glm::vec3(0,0,1);
            else               forward /= spd;

            // Up par défaut Y, sauf si forward est colinéaire à Y
            glm::vec3 worldUp = (std::abs(forward.y) > 0.99f)
                                ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
            glm::vec3 right   = glm::normalize(glm::cross(worldUp, forward));
            glm::vec3 up      = glm::cross(forward, right);

            // Matrice modèle : rotation + translation + échelle
            float s = params.fishScale;
            glm::mat4 model = glm::mat4(
                glm::vec4(right   * s, 0.0f),
                glm::vec4(up      * s, 0.0f),
                glm::vec4(forward * s, 0.0f),
                glm::vec4(fish.pos,    1.0f)
            );

            // Aplatir la mat4 column-major
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    data.push_back(model[col][row]);

            // Couleur du banc
            data.push_back(school.color.r);
            data.push_back(school.color.g);
            data.push_back(school.color.b);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instVbo);
    glBufferData(GL_ARRAY_BUFFER, data.size()*sizeof(float),
                 data.data(), GL_DYNAMIC_DRAW);
}

// ===========================================================================
int FishSystem::totalFish() const {
    int n = 0;
    for (const auto& s : m_schools) n += (int)s.fish.size();
    return n;
}

// ===========================================================================
void FishSystem::draw(const glm::mat4& view, const glm::mat4& proj,
                      const glm::vec3& lightDir, const glm::vec3& cameraPos) {
    if (!params.enabled || totalFish() == 0) return;

    m_shader.use();
    m_shader.setMat4("uView",       view);
    m_shader.setMat4("uProjection", proj);
    m_shader.setVec3("uLightDir",   lightDir);
    m_shader.setVec3("uCameraPos",  cameraPos);

    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT,
                            nullptr, (GLsizei)totalFish());
    glBindVertexArray(0);
    m_shader.unuse();
}