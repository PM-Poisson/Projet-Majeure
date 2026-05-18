#include "ocean.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

Ocean::Ocean() {}

void Ocean::init() {
    m_shader.load("shaders/ocean.vert", "shaders/ocean.frag");
    buildGrid();
}

// ---------------------------------------------------------------------------
// Grille reguliere XZ, Y = 0
// Format sommet : position(3) + normale(3) + uv(2) = 8 floats
// (identique a Mesh::upload attendu par votre renderer/mesh)
// ---------------------------------------------------------------------------
void Ocean::buildGrid() {
    const int   N     = gridResolution;
    const float half  = worldSize * 0.5f;
    const float step  = worldSize / float(N - 1);

    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    vertices.reserve(N * N * 8);
    indices.reserve((N - 1) * (N - 1) * 6);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float pz = -half + z * step;

            float u = float(x) / float(N - 1);
            float v = float(z) / float(N - 1);

            // position
            vertices.push_back(px);
            vertices.push_back(0.0f);   // Y = 0, deformation dans le vertex shader
            vertices.push_back(pz);
            // normale (vers le haut par defaut, recalculee dans le shader)
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            // uv
            vertices.push_back(u);
            vertices.push_back(v);
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
// Draw
// ---------------------------------------------------------------------------
void Ocean::draw(const glm::mat4& view,
                 const glm::mat4& proj,
                 float            time)
{
    m_shader.use();

    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4 ("uModel",      model);
    m_shader.setMat4 ("uView",       view);
    m_shader.setMat4 ("uProjection", proj);
    m_shader.setFloat("uTime",       time);
    m_shader.setFloat("uWorldSize",  worldSize);

    // Transparence legere pour l'eau
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_mesh.draw();

    glDisable(GL_BLEND);
    m_shader.unuse();
}