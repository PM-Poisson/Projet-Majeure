#include "mesh.hpp"
#include <glad/glad.h>

Mesh::Mesh() {
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void Mesh::upload(const std::vector<float>&        vertices,
                  const std::vector<unsigned int>& indices)
{
    m_indexCount = (int)indices.size();

    glBindVertexArray(m_vao);

    // VBO : donnees de sommets
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(float),
                 vertices.data(),
                 GL_DYNAMIC_DRAW);   // DYNAMIC car l'erosion modifiera les positions

    // EBO : indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Stride = 8 floats * 4 bytes = 32 bytes
    constexpr GLsizei stride = 8 * sizeof(float);

    // Attribut 0 : position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)0);

    // Attribut 1 : normale (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          (void*)(3 * sizeof(float)));

    // Attribut 2 : UV (vec2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride,
                          (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void Mesh::updateVertices(const std::vector<float>& vertices) {
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    vertices.size() * sizeof(float),
                    vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}