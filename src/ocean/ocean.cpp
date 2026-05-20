#include "ocean.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

Ocean::Ocean() {}

void Ocean::init() {
    m_shader.load("shaders/ocean.vert", "shaders/ocean.frag");
    buildGrid();
}

void Ocean::buildGrid() {
    const int   N    = gridResolution;
    const float half = worldSize * 0.5f;
    const float step = worldSize / float(N - 1);

    std::vector<float>        vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(N * N * 8);
    indices.reserve((N - 1) * (N - 1) * 6);

    for (int z = 0; z < N; ++z) {
        for (int x = 0; x < N; ++x) {
            float px = -half + x * step;
            float pz = -half + z * step;
            float u  = float(x) / float(N - 1);
            float v  = float(z) / float(N - 1);

            vertices.insert(vertices.end(), {
                px, 0.0f, pz,   // position (Y=0, deformee dans le vertex shader)
                0.0f, 1.0f, 0.0f, // normale initiale (recalculee dans le shader)
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

void Ocean::draw(const glm::mat4& view,
                 const glm::mat4& proj,
                 float            time,
                 const glm::vec3& lightDir,
                 const glm::vec3& cameraPos)
{
    m_shader.use();

    glm::mat4 model = glm::mat4(1.0f);
    m_shader.setMat4 ("uModel",      model);
    m_shader.setMat4 ("uView",       view);
    m_shader.setMat4 ("uProjection", proj);
    m_shader.setFloat("uTime",       time);
    m_shader.setVec3 ("uLightDir",   lightDir);
    m_shader.setVec3 ("uCameraPos",  cameraPos);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_mesh.draw();
    glDisable(GL_BLEND);

    m_shader.unuse();
}