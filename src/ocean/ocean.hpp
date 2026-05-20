#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "../renderer/shader.hpp"
#include "../renderer/mesh.hpp"

class Ocean {
public:
    Ocean();

    void init();

    void draw(const glm::mat4& view,
              const glm::mat4& proj,
              float            time,
              const glm::vec3& lightDir,
              const glm::vec3& cameraPos);

    int   gridResolution = 120;   // plus élevé = vagues plus lisses
    float worldSize      = 200.0f;

private:
    Shader m_shader;
    Mesh   m_mesh;
    void   buildGrid();
};